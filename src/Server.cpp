#include "../include/Server.hpp"
#include "http/ResponseHandler.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "utils/Common.hpp"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

/**
 * @brief Constructeur de la classe Server
 */
Server::Server(int port, const ServerConfig& config) 
    : server_socket()
    , port(port)
    , running(false)
    , server_config(config)
    , route_handler(config.root_directory, config) {
}

/**
 * @brief Destructeur de la classe Server
 */
Server::~Server() {
    if (running) {
        server_socket.close();
    }
    client_requests.clear();
}

/**
 * @brief Initialise le serveur
 */
void Server::initialize() {
    try {
        server_socket.create();
        server_socket.setReuseAddr(true);
        server_socket.bind(port);
        server_socket.listen();
        server_socket.setNonBlocking(true);
        
        running = true;
    } catch (const std::exception& e) {
        LOG_ERROR("Server initialization error: " << e.what());
        throw;
    }
}

/**
 * @brief Arrête le serveur
 */
void Server::stop() {
    running = false;
}

/**
 * @brief Accepte une nouvelle connexion client
 * @return Le descripteur du nouveau client ou -1 en cas d'erreur
 */
int Server::acceptNewConnection() {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    int client_fd = accept(server_socket.getFd(), (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }
        return -1;
    }
    
    // Configurer le socket client comme non-bloquant
    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    
    // Obtenir et afficher l'adresse IP du client
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    LOG_NETWORK("Client " << client_ip << " [" << client_fd << "]");
    
    // Initialiser la requête pour ce client
    client_requests[client_fd] = "";
    
    return client_fd;
}

/**
 * @brief Ferme une connexion client
 */
void Server::closeClientConnection(int client_fd) {
    if (client_fd >= 0) {
        close(client_fd);
        client_requests.erase(client_fd);
        // Pas besoin de log quand un client se déconnecte
    }
}

/**
 * @brief Traite les données reçues d'un client
 * @return false si la connexion doit être fermée, true sinon
 */
bool Server::handleClientData(int client_fd) {
    char buffer[BUFFER_SIZE];
    std::string& raw_request = client_requests[client_fd];
    size_t content_length = 0;
    bool headers_complete = false;
    int retry_count = 0;
    const int MAX_RETRIES = 10; // Réduit car 1000 est excessif
    
    while (true) {
        // Recevoir les données
        int nbytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (nbytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                retry_count++;
                if (retry_count > MAX_RETRIES) {
                    LOG_ERROR("Max retries reached while reading from client");
                    return false;
                }
                usleep(1000); // Attendre 1ms avant de réessayer
                continue;
            }
            LOG_ERROR("Error reading from client: " << strerror(errno));
            return false;
        } else if (nbytes == 0) {
            // Client a fermé la connexion
            LOG_NETWORK("Client closed connection, fd: " << client_fd);
            return false;
        }
        
        retry_count = 0; // Réinitialiser le compteur après une lecture réussie
        raw_request.append(buffer, nbytes);
        
        // Si nous n'avons pas encore trouvé la fin des headers
        if (!headers_complete) {
            size_t headers_end = raw_request.find("\r\n\r\n");
            if (headers_end != std::string::npos) {
                headers_complete = true;
                
                // Chercher Content-Length dans les headers
                size_t cl_pos = raw_request.find("Content-Length:");
                if (cl_pos != std::string::npos) {
                    size_t cl_end = raw_request.find("\r\n", cl_pos);
                    if (cl_end != std::string::npos) {
                        std::string cl_str = raw_request.substr(cl_pos + 15, cl_end - (cl_pos + 15));
                        // Supprimer les espaces
                        cl_str.erase(0, cl_str.find_first_not_of(" \t"));
                        cl_str.erase(cl_str.find_last_not_of(" \t") + 1);
                        
                        std::stringstream ss(cl_str);
                        ss >> content_length;
                    }
                }
                
                // Vérifier si on a déjà reçu toutes les données
                if (content_length == 0 || raw_request.size() >= (headers_end + 4 + content_length)) {
                    break; // On a toutes les données nécessaires
                }
            }
        } else {
            // Vérifier si on a reçu toutes les données du body
            size_t headers_end = raw_request.find("\r\n\r\n");
            if (headers_end != std::string::npos && raw_request.size() >= (headers_end + 4 + content_length)) {
                break; // On a toutes les données nécessaires
            }
        }
    }
    
    // Traiter la requête complète
    HttpRequest request;

    // Extraire l'URI pour pouvoir déterminer la taille maximale du corps autorisée
    std::string method, uri, version;
    size_t first_line_end = raw_request.find("\r\n");
    if (first_line_end != std::string::npos) {
        std::string request_line = raw_request.substr(0, first_line_end);
        std::istringstream iss(request_line);
        iss >> method >> uri >> version;
        
        // Si on a pu extraire l'URI, chercher la configuration de location correspondante
        if (!uri.empty()) {
            const LocationConfig* location = route_handler.findMatchingLocation(uri);
            if (location) {
                // Définir la taille maximale du corps autorisée pour cette location
                request.setMaxBodySize(location->client_max_body_size);
                // LOG désactivé pour réduire le bruit
                // LOG_INFO("Setting max body size for " << uri << " to " << location->client_max_body_size << " bytes");
            }
        }
    }

    if (request.parse(raw_request)) {
        // Nettoyer la requête après traitement
        client_requests[client_fd] = "";
        
        try {
            sendHttpResponse(client_fd, request);
        } catch (const std::exception& e) {
            LOG_ERROR("Error sending response: " << e.what());
            return false;
        }
    } else {
        // Requête invalide
        LOG_ERROR("Invalid HTTP request format");
        
        // Créer une réponse d'erreur 400 Bad Request
        HttpResponse response;
        response.setStatus(400);
        response.setHeader("Connection", "close");
        
        std::string error_body = "<html><body><h1>400 Bad Request</h1></body></html>";
        response.setBody(error_body);
        
        // Journaliser la réponse d'erreur avec un format uniforme
        std::cout << YELLOW << "→ ERROR" << RESET << " Invalid Request" << RESET << std::endl;
        std::cout << RED << "  ↳ 400 • Bad Request" << RESET << std::endl;
        
        // Envoyer la réponse d'erreur
        try {
            std::string http_response = response.build();
            send(client_fd, http_response.c_str(), http_response.size(), 0);
        } catch (const std::exception& e) {
            LOG_ERROR("Error sending error response: " << e.what());
        }
        
        // Nettoyer la requête
        client_requests[client_fd] = "";
        return false;
    }
    
    return true;
}

/**
 * @brief Envoie une réponse HTTP au client
 */
void Server::sendHttpResponse(int client_fd, const HttpRequest& request) {
    try {
        // Traiter la requête avec le routeur
        HttpResponse response = route_handler.processRequest(request);
        
        // Journaliser la requête et la réponse avec un format uniforme
        std::cout << 
        (std::string(request.getMethod()) == "GET" ? COLOR_GET : 
         std::string(request.getMethod()) == "POST" ? COLOR_POST : 
         std::string(request.getMethod()) == "DELETE" ? COLOR_DELETE : YELLOW) 
        << "→ " << request.getMethod() << RESET << " " << request.getUri() << RESET << std::endl;
        
        std::cout << 
        (response.getStatus() >= 200 && response.getStatus() < 300 ? GREEN : 
         response.getStatus() >= 400 ? RED : BLUE) 
        << "  ↳ " << response.getStatus() << " • " << response.getStatusMessage() << RESET << std::endl;
        
        // Convertir la réponse en chaîne et l'envoyer
        std::string http_response = response.build();
        
        // Envoyer la réponse au client
        ssize_t sent = send(client_fd, http_response.c_str(), http_response.size(), 0);
        
        if (sent < 0) {
            LOG_ERROR("Failed to send response: " << strerror(errno));
        } else {
            // Si c'est une requête "Connection: close", fermer la connexion
            if (request.getHeader("connection") == "close" || response.getHeader("Connection") == "close") {
                return;
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error processing request: " << e.what());
        
        // Créer une réponse d'erreur 500
        HttpResponse error_response;
        error_response.setStatus(500);
        error_response.setHeader("Connection", "close");
        
        std::string error_body = "<html><body><h1>500 Internal Server Error</h1></body></html>";
        error_response.setBody(error_body);
        
        // Journaliser la réponse d'erreur avec le même format uniforme
        std::cout << 
        (std::string(request.getMethod()) == "GET" ? COLOR_GET : 
         std::string(request.getMethod()) == "POST" ? COLOR_POST : 
         std::string(request.getMethod()) == "DELETE" ? COLOR_DELETE : YELLOW) 
        << "→ " << request.getMethod() << RESET << " " << request.getUri() << RESET << std::endl;
        std::cout << RED << "  ↳ 500 • Internal Server Error" << RESET << std::endl;
        
        // Envoyer la réponse d'erreur
        std::string http_response = error_response.build();
        ssize_t sent = send(client_fd, http_response.c_str(), http_response.size(), 0);
        
        if (sent < 0) {
            LOG_ERROR("Failed to send error response: " << strerror(errno));
        }
    }
}

/**
 * @brief Récupère le descripteur de fichier du socket serveur
 */
int Server::getSocketFd() const {
    return server_socket.getFd();
}

/**
 * @brief Vérifie si le descripteur donné correspond au socket serveur
 */
bool Server::matchesSocketFd(int fd) const {
    return server_socket.getFd() == fd;
}