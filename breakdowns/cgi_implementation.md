## ÉTAPE 7 - IMPLÉMENTATION CGI

### Objectif
Mettre en place un mécanisme pour exécuter des scripts CGI (comme PHP, Python) et intégrer leurs résultats dans les réponses HTTP.

### Fonctionnalités clés
- Détecter les requêtes destinées aux scripts CGI
- Configurer l'environnement CGI selon la RFC
- Gérer l'exécution du script via fork/exec
- Traiter la sortie du script pour construire la réponse HTTP

### Détails techniques

1. **Structure de données pour la configuration CGI**:
```cpp
struct CGIConfig {
    std::string extension;         // Ex: ".php"
    std::string interpreter_path;  // Ex: "/usr/bin/php-cgi"
};

Préparation des variables d'environnement:

cppCopierstd::map<std::string, std::string> prepareCGIEnvironment(const HttpRequest& request, const std::string& script_path, const ServerConfig& server_config) {
    std::map<std::string, std::string> env;
    
    // Variables standard CGI
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["SERVER_SOFTWARE"] = "webserv/1.0";
    
    // Extraire le nom d'hôte et le port de l'en-tête Host
    std::string host = request.headers.count("host") ? request.headers.at("host") : server_config.host;
    std::string server_name = host;
    std::string server_port = std::to_string(server_config.port);
    
    size_t colon_pos = host.find(':');
    if (colon_pos != std::string::npos) {
        server_name = host.substr(0, colon_pos);
        server_port = host.substr(colon_pos + 1);
    }
    
    env["SERVER_NAME"] = server_name;
    env["SERVER_PORT"] = server_port;
    
    // Variables spécifiques à la requête
    env["REQUEST_METHOD"] = request.method;
    env["PATH_INFO"] = request.uri;
    env["PATH_TRANSLATED"] = script_path;
    env["SCRIPT_FILENAME"] = script_path;
    env["SCRIPT_NAME"] = request.uri;
    env["QUERY_STRING"] = request.query_string;
    env["REMOTE_ADDR"] = "127.0.0.1"; // Adresse du client (simulée ici)
    
    // Content-Type et Content-Length (pour les requêtes POST)
    if (request.method == "POST") {
        if (request.headers.count("content-type")) {
            env["CONTENT_TYPE"] = request.headers.at("content-type");
        }
        if (request.headers.count("content-length")) {
            env["CONTENT_LENGTH"] = request.headers.at("content-length");
        } else if (!request.body.empty()) {
            env["CONTENT_LENGTH"] = std::to_string(request.body.size());
        }
    }
    
    // Autres headers HTTP
    for (const auto& header : request.headers) {
        std::string name = "HTTP_" + header.first;
        // Convertir en majuscules et remplacer '-' par '_'
        for (size_t i = 0; i < name.size(); ++i) {
            if (name[i] == '-') {
                name[i] = '_';
            } else {
                name[i] = std::toupper(name[i]);
            }
        }
        env[name] = header.second;
    }
    
    // Ajouter les variables form si présentes
    for (const auto& form_pair : request.form_values) {
        env[form_pair.first] = form_pair.second;
    }
    
    return env;
}

Détection des scripts CGI:

cppCopierbool isCGIScript(const std::string& path, const LocationConfig& location) {
    // Si l'extension est vide, ce n'est pas un script CGI
    if (location.cgi_extension.empty()) {
        return false;
    }
    
    // Vérifier l'extension du fichier
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return false;
    }
    
    std::string extension = path.substr(dot_pos);
    return extension == location.cgi_extension;
}

Exécution du script CGI:

cppCopierHttpResponse executeCGI(const HttpRequest& request, const std::string& script_path, const CGIConfig& cgi_config, const ServerConfig& server_config) {
    HttpResponse response;
    
    // Vérifier que le script existe et est exécutable
    if (access(script_path.c_str(), F_OK | X_OK) != 0) {
        return createErrorResponse(404, "CGI script not found or not executable");
    }
    
    // Préparer l'environnement
    std::map<std::string, std::string> env = prepareCGIEnvironment(request, script_path, server_config);
    
    // Trouver le répertoire du script pour chdir
    std::string script_dir = script_path.substr(0, script_path.find_last_of('/'));
    
    // Créer les pipes pour communiquer avec le processus CGI
    int input_pipe[2];  // Parent -> Child (stdin du CGI)
    int output_pipe[2]; // Child -> Parent (stdout du CGI)
    
    if (pipe(input_pipe) < 0 || pipe(output_pipe) < 0) {
        return createErrorResponse(500, "Failed to create pipes for CGI");
    }
    
    // Fork pour créer un processus enfant
    pid_t pid = fork();
    
    if (pid < 0) {
        // Erreur de fork
        close(input_pipe[0]);
        close(input_pipe[1]);
        close(output_pipe[0]);
        close(output_pipe[1]);
        return createErrorResponse(500, "Failed to fork for CGI execution");
    } 
    else if (pid == 0) {
        // Processus enfant
        
        // Changer de répertoire de travail
        if (chdir(script_dir.c_str()) < 0) {
            exit(1);
        }
        
        // Rediriger stdin depuis le pipe d'entrée
        close(input_pipe[1]); // Fermer l'extrémité d'écriture
        if (dup2(input_pipe[0], STDIN_FILENO) < 0) {
            exit(1);
        }
        close(input_pipe[0]);
        
        // Rediriger stdout vers le pipe de sortie
        close(output_pipe[0]); // Fermer l'extrémité de lecture
        if (dup2(output_pipe[1], STDOUT_FILENO) < 0) {
            exit(1);
        }
        close(output_pipe[1]);
        
        // Préparer les arguments pour execve
        char* argv[3];
        argv[0] = strdup(cgi_config.interpreter_path.c_str());
        argv[1] = strdup(script_path.c_str());
        argv[2] = NULL;
        
        // Préparer les variables d'environnement pour execve
        std::vector<char*> envp;
        for (const auto& pair : env) {
            std::string entry = pair.first + "=" + pair.second;
            envp.push_back(strdup(entry.c_str()));
        }
        envp.push_back(NULL);
        
        // Exécuter le script CGI
        execve(cgi_config.interpreter_path.c_str(), argv, envp.data());
        
        // Si execve échoue
        exit(1);
    } 
    else {
        // Processus parent
        
        // Fermer les extrémités non utilisées des pipes
        close(input_pipe[0]);
        close(output_pipe[1]);
        
        // Configurer un timeout pour l'exécution du CGI
        alarm(CGI_TIMEOUT);
        
        // Envoyer le body de la requête au script (pour les requêtes POST)
        if (request.method == "POST" && !request.body.empty()) {
            ssize_t written = write(input_pipe[1], request.body.c_str(), request.body.size());
            if (written < 0 || static_cast<size_t>(written) != request.body.size()) {
                // Erreur d'écriture
                close(input_pipe[1]);
                close(output_pipe[0]);
                kill(pid, SIGTERM);
                waitpid(pid, NULL, 0);
                return createErrorResponse(500, "Failed to send data to CGI script");
            }
        }
        close(input_pipe[1]); // Signal EOF au script
        
        // Lire la réponse du script
        std::string cgi_output;
        char buffer[4096];
        ssize_t bytes_read;
        
        // Configurer le pipe de sortie en mode non-bloquant
        fcntl(output_pipe[0], F_SETFL, O_NONBLOCK);
        
        // Structure pour poll()
        struct pollfd pfd;
        pfd.fd = output_pipe[0];
        pfd.events = POLLIN;
        
        // Lire avec timeout
        while (true) {
            int poll_result = poll(&pfd, 1, CGI_POLL_TIMEOUT);
            
            if (poll_result == 0) {
                // Timeout
                close(output_pipe[0]);
                kill(pid, SIGTERM);
                waitpid(pid, NULL, 0);
                return createErrorResponse(504, "CGI script execution timed out");
            }
            else if (poll_result < 0) {
                // Erreur
                close(output_pipe[0]);
                kill(pid, SIGTERM);
                waitpid(pid, NULL, 0);
                return createErrorResponse(500, "Error waiting for CGI script output");
            }
            
            if (pfd.revents & POLLIN) {
                bytes_read = read(output_pipe[0], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    cgi_output += buffer;
                }
                else if (bytes_read == 0) {
                    // EOF
                    break;
                }
                else if (bytes_read < 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        // Erreur réelle
                        close(output_pipe[0]);
                        kill(pid, SIGTERM);
                        waitpid(pid, NULL, 0);
                        return createErrorResponse(500, "Error reading CGI script output");
                    }
                    // Pas de données disponibles pour le moment, continuer
                }
            }
        }
        close(output_pipe[0]);
        
        // Annuler l'alarme
        alarm(0);
        
        // Attendre que le processus enfant se termine
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // Succès
            return parseCGIOutput(cgi_output);
        } else {
            // Échec de l'exécution
            return createErrorResponse(500, "CGI script execution failed");
        }
    }
}

Parsing de la sortie CGI:

cppCopierHttpResponse parseCGIOutput(const std::string& cgi_output) {
    HttpResponse response;
    
    // Chercher la séparation entre les headers et le body
    size_t header_end = cgi_output.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        // Pas de séparation claire, considérer tout comme body
        response.setStatus(200);
        response.setBody(cgi_output, "text/html");
        return response;
    }
    
    // Extraire et parser les headers
    std::string headers_section = cgi_output.substr(0, header_end);
    std::string body_section = cgi_output.substr(header_end + 4);
    
    std::istringstream headers_stream(headers_section);
    std::string header_line;
    bool status_set = false;
    
    while (std::getline(headers_stream, header_line)) {
        // Supprimer le \r à la fin
        if (!header_line.empty() && header_line.back() == '\r') {
            header_line.pop_back();
        }
        
        // Ignorer les lignes vides
        if (header_line.empty()) {
            continue;
        }
        
        size_t colon_pos = header_line.find(":");
        if (colon_pos != std::string::npos) {
            std::string name = header_line.substr(0, colon_pos);
            std::string value = header_line.substr(colon_pos + 1);
            
            // Supprimer les espaces en début/fin
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Traiter les headers spéciaux
            if (name == "Status") {
                int status_code = std::stoi(value.substr(0, value.find(" ")));
                response.setStatus(status_code);
                status_set = true;
            } else if (name == "Location") {
                response.setStatus(302); // Found / Redirection temporaire
                response.setHeader(name, value);
                status_set = true;
            } else {
                response.setHeader(name, value);
            }
        }
    }
    
    // Si aucun status n'est défini, utiliser 200 OK
    if (!status_set) {
        response.setStatus(200);
    }
    
    // Traiter le corps de la réponse
    if (!response.headers.count("Content-Type")) {
        response.setHeader("Content-Type", "text/html");
    }
    
    response.body = body_section;
    
    // Actualiser Content-Length si pas déjà présent
    if (!response.headers.count("Content-Length")) {
        response.setHeader("Content-Length", std::to_string(body_section.size()));
    }
    
    return response;
}

Gestion du handler des signaux pour le timeout:

cppCopiervoid cgiAlarmHandler(int) {
    // Ce handler sera appelé si l'alarme expire
    // Ne rien faire ici, car le code principal vérifiera si l'exécution prend trop de temps
}

void setupCGISignalHandlers() {
    struct sigaction sa;
    sa.sa_handler = cgiAlarmHandler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);
}

Routage des requêtes vers le handler CGI:

cppCopierHttpResponse handleRequest(const HttpRequest& request, const WebservConfig& config) {
    // Sélectionner le serveur virtuel approprié
    const ServerConfig& server = selectVirtualServer(config, 
                                                   getHostFromRequest(request), 
                                                   getPortFromRequest(request));
    
    // Sélectionner la location
    const LocationConfig& location = selectLocation(server, request.uri);
    
    // Vérifier si la méthode est autorisée
    if (!isMethodAllowed(request.method, location)) {
        return createErrorResponse(405, "Method Not Allowed");
    }
    
    // Vérifier s'il s'agit d'une redirection
    if (!location.redirect_url.empty()) {
        HttpResponse redirect;
        redirect.setRedirect(location.redirect_url);
        return redirect;
    }
    
    // Résoudre le chemin du fichier
    std::string file_path = resolveFilePath(request.uri, server, location);
    
    // Vérifier s'il s'agit d'un script CGI
    if (isCGIScript(file_path, location)) {
        // Configurer le CGI
        CGIConfig cgi_config;
        cgi_config.extension = location.cgi_extension;
        cgi_config.interpreter_path = getCGIInterpreterPath(location.cgi_extension);
        
        // Exécuter le script CGI
        return executeCGI(request, file_path, cgi_config, server);
    }
    
    // Sinon, traiter comme un fichier statique
    return handleStaticFileRequest(request, file_path, server, location);
}

Détermination de l'interpréteur CGI:

cppCopierstd::string getCGIInterpreterPath(const std::string& extension) {
    static const std::map<std::string, std::string> interpreters = {
        {".php", "/usr/bin/php-cgi"},
        {".py", "/usr/bin/python"},
        {".pl", "/usr/bin/perl"},
        {".rb", "/usr/bin/ruby"}
    };
    
    auto it = interpreters.find(extension);
    if (it != interpreters.end()) {
        return it->second;
    }
    
    // Par défaut, renvoyer le chemin PHP (le plus commun)
    return "/usr/bin/php-cgi";
}
Points de validation

 Détecte correctement les requêtes destinées aux scripts CGI
 Configure l'environnement CGI selon les standards
 Transmet correctement les données POST aux scripts
 Traite correctement la sortie des scripts
 Gère les en-têtes générés par le script CGI
 Implémente un mécanisme de timeout pour les scripts lents
 Nettoie toutes les ressources en cas d'erreur

Tests recommandés

Test avec un script PHP simple:

phpCopier<?php
// info.php
phpinfo();
?>
bashCopiercurl -v http://localhost:8080/cgi-bin/info.php

Test avec des paramètres GET:

phpCopier<?php
// echo.php
echo "GET parameters:\n";
foreach ($_GET as $key => $value) {
    echo "$key: $value\n";
}
?>
bashCopiercurl -v "http://localhost:8080/cgi-bin/echo.php?name=value&foo=bar"

Test avec des données POST:

phpCopier<?php
// post.php
echo "POST data:\n";
foreach ($_POST as $key => $value) {
    echo "$key: $value\n";
}
echo "Raw POST data: " . file_get_contents("php://input");
?>
bashCopiercurl -v -X POST -d "name=value&foo=bar" http://localhost:8080/cgi-bin/post.php

Test avec un upload de fichier:

phpCopier<?php
// upload.php
echo "Uploaded files:\n";
var_dump($_FILES);

if (!empty($_FILES)) {
    $target_dir = "/tmp/uploads/";
    foreach ($_FILES as $file) {
        $target_file = $target_dir . basename($file["name"]);
        if (move_uploaded_file($file["tmp_name"], $target_file)) {
            echo "File uploaded to $target_file\n";
        } else {
            echo "Error uploading file\n";
        }
    }
}
?>
bashCopiercurl -v -F "file=@test.txt" http://localhost:8080/cgi-bin/upload.php

Test de timeout:

phpCopier<?php
// slow.php
sleep(10); // Script lent qui dépasse le timeout
echo "This should not be displayed";
?>
bashCopiercurl -v http://localhost:8080/cgi-bin/slow.php

Test avec un script inexistant:

bashCopiercurl -v http://localhost:8080/cgi-bin/nonexistent.php
Bonnes pratiques

Limiter le temps d'exécution des scripts CGI
Nettoyer toutes les ressources, même en cas d'erreur
Valider les chemins des scripts pour éviter les vulnérabilités
Journaliser les erreurs d'exécution pour faciliter le débogage
Assurer que le processus enfant est toujours correctement terminé
Supporter différents interpréteurs pour différentes extensions de fichiers
Implémenter des mécanismes de sécurité (limiter l'accès aux répertoires sensibles)