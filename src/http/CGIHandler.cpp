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

CGIHandler::CGIHandler(const HttpRequest& req, const std::string& script_path, const std::string& interpreter)
    : request_(req), script_path_(script_path), interpreter_(interpreter) {
}

CGIHandler::~CGIHandler() {
}

HttpResponse CGIHandler::executeCGI() {
    try {
        if (!isCGIScript()) {
            std::cerr << "Script not found: " << script_path_ << std::endl;
            return HttpResponse::createError(404, "Script not found");
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "Script not executable: " << script_path_ << std::endl;
        return HttpResponse::createError(403, "Script not executable");
    }

    int pipe_in[2];  // Pour écrire vers le script
    int pipe_out[2]; // Pour lire depuis le script

    if (pipe(pipe_in) < 0 || pipe(pipe_out) < 0) {
        std::cerr << "Failed to create pipes" << std::endl;
        return HttpResponse::createError(500, "Failed to create pipes");
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Failed to fork process" << std::endl;
        close(pipe_in[0]); close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);
        return HttpResponse::createError(500, "Failed to fork process");
    }

    if (pid == 0) {
        // Processus enfant
        if (!executeCGIScript(pipe_in, pipe_out)) {
            std::cerr << "Failed to execute CGI script" << std::endl;
            exit(1);
        }
        exit(0);
    }

    // Processus parent
    close(pipe_in[0]);  // Ferme l'extrémité de lecture du pipe d'entrée
    close(pipe_out[1]); // Ferme l'extrémité d'écriture du pipe de sortie

    // Envoie les données POST au script si nécessaire
    if (request_.getMethod() == "POST") {
        const std::string& body = request_.getBody();
        if (write(pipe_in[1], body.c_str(), body.length()) < 0) {
            std::cerr << "Failed to write to CGI script" << std::endl;
            close(pipe_in[1]);
            close(pipe_out[0]);
            return HttpResponse::createError(500, "Failed to write to CGI script");
        }
    }
    close(pipe_in[1]); // Ferme le pipe d'entrée après l'écriture

    // Lit la sortie du script
    std::string output;
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(pipe_out[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, bytes_read);
    }
    close(pipe_out[0]);

    // Attend la fin du processus enfant
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status != 0) {
            std::cerr << "CGI script exited with status " << exit_status << std::endl;
            return HttpResponse::createError(500, "CGI script execution failed");
        }
    } else {
        std::cerr << "CGI script terminated abnormally" << std::endl;
        return HttpResponse::createError(500, "CGI script terminated abnormally");
    }

    if (output.empty()) {
        std::cerr << "CGI script produced no output" << std::endl;
        return HttpResponse::createError(500, "CGI script produced no output");
    }

    // Afficher la sortie brute pour le débogage
    std::cerr << "=== Raw CGI output ===" << std::endl;
    std::cerr << output << std::endl;
    std::cerr << "=== End of raw CGI output ===" << std::endl;

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

    // Variables d'environnement standard CGI
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("SERVER_SOFTWARE=webserv/1.0");
    env.push_back("REQUEST_METHOD=" + request_.getMethod());
    env.push_back("SCRIPT_NAME=" + script_path_);
    env.push_back("QUERY_STRING=" + request_.getQueryString());
    env.push_back("REDIRECT_STATUS=200");
    env.push_back("SCRIPT_FILENAME=" + script_path_);
    env.push_back("DOCUMENT_ROOT=/home/j/Desktop/GITHUB-42/42-webserv/www");
    env.push_back("SERVER_NAME=localhost");
    env.push_back("SERVER_PORT=8080");
    env.push_back("PATH=/usr/local/bin:/usr/bin:/bin");

    // En-têtes de la requête
    const std::map<std::string, std::string>& headers = request_.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        std::string header_name = it->first;
        std::transform(header_name.begin(), header_name.end(), header_name.begin(), ::toupper);
        std::string env_name = "HTTP_" + header_name;
        std::replace(env_name.begin(), env_name.end(), '-', '_');
        env.push_back(env_name + "=" + it->second);
    }

    // Content-Length et Content-Type pour les requêtes POST
    if (request_.getMethod() == "POST") {
        env.push_back("CONTENT_LENGTH=" + request_.getHeader("Content-Length"));
        env.push_back("CONTENT_TYPE=" + request_.getHeader("Content-Type"));
    }

    return env;
}

HttpResponse CGIHandler::parseCGIOutput(const std::string& output) {
    HttpResponse response;
    std::istringstream iss(output);
    std::string line;
    std::string body;
    bool headers_done = false;

    // Parse les en-têtes et le corps
    while (std::getline(iss, line)) {
        // Supprimer le retour chariot à la fin de la ligne si présent
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
                // Supprime les espaces au début et à la fin
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                if (key == "Status") {
                    response.setStatus(atoi(value.c_str()));
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
        // Aucun en-tête trouvé, considère tout comme le corps
        body = output;
    }

    // Si aucun Content-Type n'est défini, utilise text/html par défaut
    std::string content_type = "text/html";
    std::map<std::string, std::string>::const_iterator it = response.getHeaders().find("Content-Type");
    if (it != response.getHeaders().end()) {
        content_type = it->second;
    }

    // Supprimer les espaces et retours à la ligne superflus au début et à la fin du corps
    while (!body.empty() && (body[0] == ' ' || body[0] == '\n' || body[0] == '\r')) {
        body.erase(0, 1);
    }
    while (!body.empty() && (body[body.length() - 1] == ' ' || body[body.length() - 1] == '\n' || body[body.length() - 1] == '\r')) {
        body.erase(body.length() - 1);
    }

    // Définir le corps avec le bon Content-Type
    response.setBody(body, content_type);

    if (!response.getStatusCode()) {
        response.setStatus(200);
    }

    return response;
}

bool CGIHandler::isCGIScript() const {
    // D'abord vérifier si le fichier existe
    if (access(script_path_.c_str(), F_OK) != 0) {
        std::cerr << "Script " << script_path_ << " does not exist" << std::endl;
        return false;
    }
    
    // Ensuite vérifier s'il est exécutable
    if (access(script_path_.c_str(), X_OK) != 0) {
        std::cerr << "Script " << script_path_ << " is not executable" << std::endl;
        throw std::runtime_error("Script is not executable");
    }
    
    return true;
} 