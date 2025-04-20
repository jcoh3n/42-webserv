#include "http/CGIHandler.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/utils/FileUtils.hpp"
#include "utils/Common.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <limits.h>  // Pour PATH_MAX
#include <cerrno>
#include <sys/time.h>
#include <sys/types.h>
#include <poll.h>

CGIHandler::CGIHandler(const HttpRequest& req, const std::string& script_path, const std::string& interpreter)
    : request_(req), script_path_(script_path), interpreter_(interpreter), root_directory_("") {
}

CGIHandler::CGIHandler(const HttpRequest& req, const std::string& script_path, const std::string& interpreter,
                       const std::string& root_dir, const std::map<int, std::string>& error_pages)
    : request_(req), script_path_(script_path), interpreter_(interpreter), 
      root_directory_(root_dir), error_pages_(error_pages) {
}

CGIHandler::~CGIHandler() {
}

HttpResponse CGIHandler::executeCGI() {
    try {
        if (!isCGIScript()) {
            LOG_CGI_ERROR("Script not found: " + script_path_);
            return serveErrorPage(404, "Script not found");
        }
    } catch (const std::runtime_error& e) {
        LOG_CGI_ERROR("Script not executable: " + script_path_);
        return serveErrorPage(403, "Script not executable");
    }

    int pipe_in[2];
    int pipe_out[2];

    if (pipe(pipe_in) < 0 || pipe(pipe_out) < 0) {
        LOG_CGI_ERROR("Failed to create pipes");
        return serveErrorPage(500, "Failed to create pipes");
    }

    pid_t pid = fork();
    if (pid < 0) {
        LOG_CGI_ERROR("Failed to fork process");
        close(pipe_in[0]); close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);
        return serveErrorPage(500, "Failed to fork process");
    }

    if (pid == 0) {
        alarm(CGI_TIMEOUT);
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
            LOG_CGI_ERROR("Failed to write to CGI script");
            close(pipe_in[1]);
            close(pipe_out[0]);
            return serveErrorPage(500, "Failed to write to CGI script");
        }
    }
    close(pipe_in[1]);

    // Make pipe_out non-blocking
    fcntl(pipe_out[0], F_SETFL, O_NONBLOCK);

    // Setup poll to detect timeout more reliably
    struct pollfd pfd;
    pfd.fd = pipe_out[0];
    pfd.events = POLLIN;

    std::string output;
    char buffer[4096];
    ssize_t bytes_read;
    int timeout_ms = CGI_TIMEOUT * 1000; // Convert to milliseconds
    bool timed_out = false;

    // Poll with timeout to read from the pipe
    while (!timed_out) {
        int poll_result = poll(&pfd, 1, timeout_ms);
        
        if (poll_result == 0) {
            // Timeout occurred
            LOG_CGI_ERROR("Script timed out (poll timeout)");
            timed_out = true;
            break;
        } else if (poll_result < 0) {
            // Error in poll
            if (errno == EINTR) {
                continue; // Interrupted, try again
            }
            LOG_CGI_ERROR("Poll failed: " + std::string(strerror(errno)));
            close(pipe_out[0]);
            kill(pid, SIGTERM);
            waitpid(pid, NULL, 0);
            return serveErrorPage(500, "Error in poll");
        }

        // Data is available to read
        bytes_read = read(pipe_out[0], buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            output.append(buffer, bytes_read);
        } else if (bytes_read == 0) {
            // End of file
            break;
        } else {
            // Error or would block
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_CGI_ERROR("Reading from pipe: " + std::string(strerror(errno)));
                close(pipe_out[0]);
                kill(pid, SIGTERM);
                waitpid(pid, NULL, 0);
                return serveErrorPage(500, "Error reading from pipe");
            }
        }
    }
    
    close(pipe_out[0]);

    // If timeout occurred, kill the process and return timeout error
    if (timed_out) {
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
        LOG_CGI_ERROR("Script timed out");
        return serveErrorPage(504, "CGI script timed out");
    }

    // Get process exit status
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status != 0) {
            char status_msg[64];
            sprintf(status_msg, "Script exited with status %d", exit_status);
            LOG_CGI_ERROR(status_msg);
            return serveErrorPage(500, "CGI script execution failed");
        }
    } 
    else if (WIFSIGNALED(status)) {
        int signal = WTERMSIG(status);
        if (signal == SIGALRM) {
            LOG_CGI_ERROR("Script timed out (SIGALRM)");
            return serveErrorPage(504, "CGI script timed out");
        } else {
            char signal_msg[64];
            sprintf(signal_msg, "Script terminated by signal %d", signal);
            LOG_CGI_ERROR(signal_msg);
            return serveErrorPage(500, "CGI script terminated by signal");
        }
    } 
    else {
        LOG_CGI_ERROR("Script terminated abnormally");
        return serveErrorPage(500, "CGI script terminated abnormally");
    }

    if (output.empty()) {
        LOG_CGI_ERROR("Script produced no output");
        return serveErrorPage(500, "CGI script produced no output");
    }

    return parseCGIOutput(output);
}

bool CGIHandler::executeCGIScript(int pipe_in[2], int pipe_out[2]) {
    // Redirige stdin vers le pipe d'entrée
    close(pipe_in[1]);
    if (dup2(pipe_in[0], STDIN_FILENO) < 0) {
        LOG_CGI_ERROR("Failed to redirect stdin");
        return false;
    }
    close(pipe_in[0]);

    // Redirige stdout vers le pipe de sortie
    close(pipe_out[0]);
    if (dup2(pipe_out[1], STDOUT_FILENO) < 0) {
        LOG_CGI_ERROR("Failed to redirect stdout");
        return false;
    }
    close(pipe_out[1]);

    // Prépare l'environnement
    std::vector<std::string> env = prepareEnvironment();
    char* envp[env.size() + 1];
    for (size_t i = 0; i < env.size(); ++i) {
        envp[i] = strdup(env[i].c_str());
        if (!envp[i]) {
            LOG_CGI_ERROR("Failed to allocate environment variable");
            for (size_t j = 0; j < i; ++j) {
                free(envp[j]);
            }
            return false;
        }
    }
    envp[env.size()] = NULL;

    // Prépare les arguments
    char* args[3];
    args[0] = strdup(interpreter_.c_str());
    args[1] = strdup(script_path_.c_str());
    args[2] = NULL;
    if (!args[0] || !args[1]) {
        LOG_CGI_ERROR("Failed to allocate arguments");
        free(args[0]);
        free(args[1]);
        return false;
    }

    execve(interpreter_.c_str(), args, envp);
    LOG_CGI_ERROR("Failed to execute " + interpreter_ + ": " + strerror(errno));
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

HttpResponse CGIHandler::serveErrorPage(int error_code, const std::string& message) {
    // Si nous avons un répertoire racine et des pages d'erreur configurées
    if (!root_directory_.empty() && !error_pages_.empty()) {
        // Chercher une page d'erreur personnalisée pour ce code
        std::map<int, std::string>::const_iterator it = error_pages_.find(error_code);
        if (it != error_pages_.end()) {
            // Construire le chemin complet vers la page d'erreur
            std::string error_page_path = root_directory_ + "/" + it->second;
            
            // Vérifier si le fichier existe
            if (FileUtils::fileExists(error_page_path)) {
                HttpResponse response;
                response.setStatus(error_code);
                
                // Lire le contenu du fichier
                std::ifstream file(error_page_path.c_str(), std::ios::binary);
                if (file) {
                    std::string content((std::istreambuf_iterator<char>(file)),
                                         std::istreambuf_iterator<char>());
                    file.close();
                    
                    // Détecter le type MIME en fonction de l'extension
                    std::string mime_type = "text/html";
                    if (error_page_path.find(".css") != std::string::npos) {
                        mime_type = "text/css";
                    } else if (error_page_path.find(".js") != std::string::npos) {
                        mime_type = "application/javascript";
                    } else if (error_page_path.find(".json") != std::string::npos) {
                        mime_type = "application/json";
                    }
                    
                    response.setHeader("Content-Type", mime_type);
                    // Désactiver le cache pour les pages d'erreur
                    response.setHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
                    response.setHeader("Pragma", "no-cache");
                    response.setBody(content);
                    return response;
                }
            }
        }
    }
    
    // Page d'erreur par défaut si aucune page personnalisée n'est disponible
    HttpResponse response = HttpResponse::createError(error_code, message);
    // Désactiver le cache pour les pages d'erreur par défaut aussi
    response.setHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    response.setHeader("Pragma", "no-cache");
    return response;
} 