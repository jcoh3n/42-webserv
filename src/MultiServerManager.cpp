#include "MultiServerManager.hpp"
#include <csignal>
#include <cstring>
#include <vector>

// Initialisation de la variable statique
MultiServerManager* MultiServerManager::instance = NULL;

/**
 * @brief Constructeur de la classe MultiServerManager
 */
MultiServerManager::MultiServerManager() 
    : poll_fds(NULL)
    , nfds(0)
    , running(false) {
    // Enregistrer l'instance pour le gestionnaire de signal
    instance = this;
    
    // Allouer de la mémoire pour le tableau poll
    poll_fds = new struct pollfd[MAX_POLL_SIZE];
    memset(poll_fds, 0, sizeof(struct pollfd) * MAX_POLL_SIZE);
}

/**
 * @brief Destructeur de la classe MultiServerManager
 */
MultiServerManager::~MultiServerManager() {
    // Ne pas appeler stopServers() ici car il est déjà appelé explicitement dans main.cpp
    // Cela évite de double-libérer des ressources
    
    // Nettoyer les ressources
    if (servers.size() > 0) {
        for (size_t i = 0; i < servers.size(); i++) {
            if (servers[i]) {
                delete servers[i];
                servers[i] = NULL;
            }
        }
        servers.clear();
    }
    
    port_to_server_index.clear();
    fd_to_server.clear();
    
    // Libérer la mémoire du tableau poll
    if (poll_fds) {
        delete[] poll_fds;
        poll_fds = NULL;
    }
    
    // Ne pas mettre instance à NULL ici car cela pourrait causer des problèmes
    // si un signal arrive pendant la destruction
}

/**
 * @brief Configure les gestionnaires de signaux
 */
void MultiServerManager::setupSignalHandlers() {
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, NULL);  // Ctrl+C
    sigaction(SIGTERM, &sa, NULL); // Signal de terminaison
}

/**
 * @brief Gestionnaire statique des signaux
 */
void MultiServerManager::signalHandler(int signal) {
    LOG_INFO("Signal " << signal << " reçu, arrêt en cours...");
    if (instance) {
        // Plutôt que d'appeler stopServers() qui pourrait causer des problèmes dans un handler de signal,
        // on arrête simplement la boucle principale pour permettre une sortie propre
        instance->running = false;
    }
}

/**
 * @brief Initialise les serveurs à partir de la configuration
 */
void MultiServerManager::initServers(const WebservConfig& config) {
    if (config.servers.empty()) {
        throw std::runtime_error("No server configured");
    }
    
    for (size_t i = 0; i < config.servers.size(); i++) {
        const ServerConfig& server_config = config.servers[i];
        int port = server_config.port;
        
        // Vérifier si ce port est déjà utilisé
        if (port_to_server_index.find(port) != port_to_server_index.end()) {
            LOG_WARNING("Server on port " << port << " already configured, skipping...");
            continue;
        }
        
        LOG_INFO("Initializing server on port " << port);
        Server* server = new Server(port, server_config);
        servers.push_back(server);
        port_to_server_index[port] = servers.size() - 1;
    }
    
    if (servers.empty()) {
        throw std::runtime_error("No valid server configured");
    }
    
    LOG_SUCCESS("Initialized " << servers.size() << " server(s)");
    
    // Configuration des gestionnaires de signaux
    setupSignalHandlers();
}

/**
 * @brief Ajoute un descripteur de fichier au tableau poll
 */
void MultiServerManager::addFdToPoll(int fd, Server* server) {
    if (nfds >= MAX_POLL_SIZE) {
        LOG_WARNING("Maximum poll size reached, cannot add more descriptors");
        return;
    }
    
    poll_fds[nfds].fd = fd;
    poll_fds[nfds].events = POLLIN;
    poll_fds[nfds].revents = 0;
    
    fd_to_server[fd] = server;
    nfds++;
}

/**
 * @brief Supprime un descripteur de fichier du tableau poll
 */
void MultiServerManager::removeFdFromPoll(int fd_index) {
    if (fd_index < 0 || fd_index >= nfds) {
        return;
    }
    
    int fd = poll_fds[fd_index].fd;
    fd_to_server.erase(fd);
    
    // Compacter le tableau en déplaçant le dernier fd à cette position
    if (fd_index < nfds - 1) {
        poll_fds[fd_index] = poll_fds[nfds - 1];
    }
    
    nfds--;
}

/**
 * @brief Récupère le serveur associé à un fd
 */
Server* MultiServerManager::getServerByFd(int fd) const {
    std::map<int, Server*>::const_iterator it = fd_to_server.find(fd);
    if (it != fd_to_server.end()) {
        return it->second;
    }
    return NULL;
}

/**
 * @brief Gère un événement de poll
 */
bool MultiServerManager::handleEvent(int index) {
    int fd = poll_fds[index].fd;
    Server* server = getServerByFd(fd);
    
    if (!server) {
        LOG_ERROR("No server associated with fd: " << fd);
        removeFdFromPoll(index);
        return false;
    }
    
    // Vérifier si c'est un socket serveur ou client
    if (server->matchesSocketFd(fd)) {
        // C'est un socket serveur, accepter la nouvelle connexion
        int client_fd = server->acceptNewConnection();
        if (client_fd >= 0) {
            // Ajouter le nouveau client au poll
            addFdToPoll(client_fd, server);
        }
    } else {
        // C'est un socket client, traiter les données
        bool keep_connection = server->handleClientData(fd);
        if (!keep_connection) {
            // Fermer la connexion si nécessaire
            server->closeClientConnection(fd);
            removeFdFromPoll(index);
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Démarre tous les serveurs
 */
void MultiServerManager::startServers() {
    if (servers.empty()) {
        throw std::runtime_error("No servers initialized");
    }
    
    // Initialiser tous les serveurs et ajouter leurs sockets au poll
    for (size_t i = 0; i < servers.size(); i++) {
        try {
            servers[i]->initialize();
            int server_fd = servers[i]->getSocketFd();
            addFdToPoll(server_fd, servers[i]);
            LOG_SUCCESS("Server listening on " << BLUE << BOLD << "http://localhost:" << servers[i]->getPort() << RESET);
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to initialize server on port " << servers[i]->getPort() << ": " << e.what());
        }
    }
    
    if (nfds == 0) {
        throw std::runtime_error("No valid server sockets to listen on");
    }
    
    running = true;
    
    // Boucle principale
    while (running) {
        int ret = poll(poll_fds, nfds, -1);
        if (ret < 0) {
            if (errno == EINTR) {
                // Interruption par un signal
                continue;
            }
            LOG_ERROR("Poll failed: " << strerror(errno));
            break;
        }
        
        // Traitement des événements
        for (int i = 0; i < nfds; i++) {
            if (poll_fds[i].revents == 0) {
                continue;
            }
            
            if (poll_fds[i].revents & POLLIN) {
                if (handleEvent(i) == false) {
                    i--; // Ajuster l'index si un fd a été supprimé
                }
            }
            
            if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                int fd = poll_fds[i].fd;
                Server* server = getServerByFd(fd);
                
                if (server && !server->matchesSocketFd(fd)) {
                    // Socket client en erreur
                    server->closeClientConnection(fd);
                    removeFdFromPoll(i);
                    i--;
                } else if (server && server->matchesSocketFd(fd)) {
                    // Erreur critique sur socket serveur
                    LOG_ERROR("Error on server socket for port " << server->getPort());
                    running = false;
                    break;
                }
            }
        }
    }
}

/**
 * @brief Arrête tous les serveurs
 */
void MultiServerManager::stopServers() {
    static bool already_stopping = false;
    if (!already_stopping) {
        already_stopping = true;
        LOG_INFO("Arrêt des serveurs...");
        
        running = false;
        
        // Fermer toutes les connexions clients
        if (poll_fds && nfds > 0) {
            for (int i = 0; i < nfds; i++) {
                if (poll_fds[i].fd >= 0) {
                    Server* server = getServerByFd(poll_fds[i].fd);
                    if (server && !server->matchesSocketFd(poll_fds[i].fd)) {
                        server->closeClientConnection(poll_fds[i].fd);
                    }
                }
            }
        }
        
        // Arrêter tous les serveurs
        for (size_t i = 0; i < servers.size(); i++) {
            if (servers[i]) {
                servers[i]->stop();
            }
        }
        
        // Vider les structures de données
        fd_to_server.clear();
        nfds = 0;
        
        LOG_SUCCESS("Tous les serveurs ont été arrêtés");
    }
} 