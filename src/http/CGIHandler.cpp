#include "http/CGIHandler.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fcntl.h>
#include <limits.h>  // Pour PATH_MAX

CGIHandler::CGIHandler(const HttpRequest& req, const std::string& script_path, const std::string& interpreter)
    : request_(req), script_path_(script_path), interpreter_(interpreter) {
}

CGIHandler::~CGIHandler() {
}

HttpResponse CGIHandler::executeCGI() {
    try {
        if (!isCGIScript()) {
            std::cerr << "[CGI] Error: Script not found: " << script_path_ << std::endl;
            return HttpResponse::createError(404, "Script not found");
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "[CGI] Error: Script not executable: " << script_path_ << std::endl;
        return HttpResponse::createError(403, "Script not executable");
    }

    int pipe_in[2];
    int pipe_out[2];

    if (pipe(pipe_in) < 0 || pipe(pipe_out) < 0) {
        std::cerr << "[CGI] Error: Failed to create pipes" << std::endl;
        return HttpResponse::createError(500, "Failed to create pipes");
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "[CGI] Error: Failed to fork process" << std::endl;
        close(pipe_in[0]); close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);
        return HttpResponse::createError(500, "Failed to fork process");
    }

    if (pid == 0) {
        if (!executeCGIScript(pipe_in, pipe_out)) {
            exit(1);
        }
        exit(0);
    }

    close(pipe_in[0]);
    close(pipe_out[1]);

    if (request_.getMethod() == "POST") {
        const std::string& body = request_.getBody();
        if (write(pipe_in[1], body.c_str(), body.length()) < 0) {
            std::cerr << "[CGI] Error: Failed to write to CGI script" << std::endl;
            close(pipe_in[1]);
            close(pipe_out[0]);
            return HttpResponse::createError(500, "Failed to write to CGI script");
        }
    }
    close(pipe_in[1]);

    std::string output;
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(pipe_out[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, bytes_read);
    }
    close(pipe_out[0]);

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status != 0) {
            std::cerr << "[CGI] Error: Script exited with status " << exit_status << std::endl;
            return HttpResponse::createError(500, "CGI script execution failed");
        }
    } else {
        std::cerr << "[CGI] Error: Script terminated abnormally" << std::endl;
        return HttpResponse::createError(500, "CGI script terminated abnormally");
    }

    if (output.empty()) {
        std::cerr << "[CGI] Error: Script produced no output" << std::endl;
        return HttpResponse::createError(500, "CGI script produced no output");
    }

    return parseCGIOutput(output);
}

bool CGIHandler::executeCGIScript(int pipe_in[2], int pipe_out[2]) {
    // Redirige stdin vers le pipe d'entrée
    close(pipe_in[1]);
    if (dup2(pipe_in[0], STDIN_FILENO) < 0) {
        std::cerr << "Failed to redirect stdin" << std::endl;
        return false;
    }
    close(pipe_in[0]);

    // Redirige stdout vers le pipe de sortie
    close(pipe_out[0]);
    if (dup2(pipe_out[1], STDOUT_FILENO) < 0) {
        std::cerr << "Failed to redirect stdout" << std::endl;
        return false;
    }
    close(pipe_out[1]);

    // Prépare l'environnement
    std::vector<std::string> env = prepareEnvironment();
    char* envp[env.size() + 1];
    for (size_t i = 0; i < env.size(); ++i) {
        envp[i] = strdup(env[i].c_str());
        if (envp[i] == NULL) {
            std::cerr << "Failed to allocate environment variable" << std::endl;
            return false;
        }
    }
    envp[env.size()] = NULL;

    // Prépare les arguments
    char* args[3];
    args[0] = strdup(interpreter_.c_str());
    args[1] = strdup(script_path_.c_str());
    args[2] = NULL;

    if (args[0] == NULL || args[1] == NULL) {
        std::cerr << "Failed to allocate arguments" << std::endl;
        return false;
    }

    // Change le répertoire de travail vers le répertoire du script
    std::string script_dir = script_path_.substr(0, script_path_.find_last_of('/'));
    if (chdir(script_dir.c_str()) < 0) {
        std::cerr << "Failed to change directory to " << script_dir << std::endl;
        return false;
    }

    // Exécute le script
    execve(interpreter_.c_str(), args, envp);

    // Si on arrive ici, c'est que execve a échoué
    std::cerr << "Failed to execute " << interpreter_ << std::endl;

    // Libère la mémoire
    for (size_t i = 0; envp[i] != NULL; ++i) {
        free(envp[i]);
    }
    free(args[0]);
    free(args[1]);

    return false;
}

std::vector<std::string> CGIHandler::prepareEnvironment() const {
    std::vector<std::string> env;

    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("SERVER_SOFTWARE=webserv/1.0");
    env.push_back("REQUEST_METHOD=" + request_.getMethod());
    
    char real_path[PATH_MAX];
    if (realpath(script_path_.c_str(), real_path) != NULL) {
        env.push_back("SCRIPT_FILENAME=" + std::string(real_path));
        env.push_back("SCRIPT_NAME=" + script_path_);
    } else {
        env.push_back("SCRIPT_FILENAME=" + script_path_);
        env.push_back("SCRIPT_NAME=" + script_path_);
    }

    std::string doc_root = "/home/j/Desktop/GITHUB-42/42-webserv/www";
    if (realpath(doc_root.c_str(), real_path) != NULL) {
        env.push_back("DOCUMENT_ROOT=" + std::string(real_path));
    } else {
        env.push_back("DOCUMENT_ROOT=" + doc_root);
    }

    std::string query_string = request_.getQueryString();
    env.push_back("QUERY_STRING=" + query_string);
    
    env.push_back("REDIRECT_STATUS=200");
    env.push_back("SERVER_NAME=localhost");
    env.push_back("SERVER_PORT=8080");
    env.push_back("REMOTE_ADDR=127.0.0.1");
    env.push_back("PATH=/usr/local/bin:/usr/bin:/bin");
    
    const std::map<std::string, std::string>& headers = request_.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        std::string header_name = it->first;
        std::transform(header_name.begin(), header_name.end(), header_name.begin(), ::toupper);
        std::string env_name = "HTTP_" + header_name;
        std::replace(env_name.begin(), env_name.end(), '-', '_');
        env.push_back(env_name + "=" + it->second);
    }

    if (request_.getMethod() == "POST") {
        std::string contentLength = request_.getHeader("Content-Length");
        std::string contentType = request_.getHeader("Content-Type");
        
        if (!contentLength.empty()) {
            env.push_back("CONTENT_LENGTH=" + contentLength);
        } else {
            std::stringstream ss;
            ss << request_.getBody().length();
            env.push_back("CONTENT_LENGTH=" + ss.str());
        }
        
        if (!contentType.empty()) {
            env.push_back("CONTENT_TYPE=" + contentType);
        }
    }

    return env;
}

HttpResponse CGIHandler::parseCGIOutput(const std::string& output) {
    HttpResponse response;
    std::istringstream iss(output);
    std::string line;
    std::string body;
    bool headers_done = false;
    bool has_status = false;
    bool has_content_type = false;

    while (std::getline(iss, line)) {
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }

        if (!headers_done) {
            if (line.empty()) {
                headers_done = true;
                continue;
            }
            
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                if (key == "Status") {
                    response.setStatus(atoi(value.c_str()));
                    has_status = true;
                } else if (key == "Content-Type") {
                    response.setHeader(key, value);
                    has_content_type = true;
                } else {
                    response.setHeader(key, value);
                }
            }
        } else {
            if (!body.empty()) {
                body += "\n";
            }
            body += line;
        }
    }

    if (!headers_done) {
        body = output;
    }

    if (!has_content_type) {
        if (body.find("<!DOCTYPE html>") != std::string::npos || 
            body.find("<html") != std::string::npos) {
            response.setHeader("Content-Type", "text/html");
        } else if (body[0] == '{' || body[0] == '[') {
            response.setHeader("Content-Type", "application/json");
        } else {
            response.setHeader("Content-Type", "text/plain");
        }
    }

    if (!has_status) {
        response.setStatus(200);
    }

    while (!body.empty() && (body[0] == '\n' || body[0] == '\r')) {
        body.erase(0, 1);
    }
    while (!body.empty() && (body[body.length() - 1] == '\n' || body[body.length() - 1] == '\r')) {
        body.erase(body.length() - 1);
    }

    response.setBody(body);
    return response;
}

bool CGIHandler::isCGIScript() const {
    if (access(script_path_.c_str(), F_OK) != 0) {
        return false;
    }
    
    if (access(script_path_.c_str(), X_OK) != 0) {
        throw std::runtime_error("Script is not executable");
    }
    
    return true;
} 