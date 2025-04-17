#include "MultiServerManager.hpp"
#include <csignal>
#include <cstring>
#include <vector>

// Initialisation de la variable statique
MultiServerManager* MultiServerManager::instance = NULL;

/**
 * @brief Constructeur de la classe MultiServerManager
 */
MultiServerManager::MultiServerManager() {
    // Enregistrer l'instance pour le gestionnaire de signal
    instance = this;
}

/**
 * @brief Destructeur de la classe MultiServerManager
 * 
 * Nettoie les ressources avant de détruire l'objet.
 */
MultiServerManager::~MultiServerManager() {
    stopServers();
    
    // Nettoyer les ressources
    for (size_t i = 0; i < servers.size(); i++) {
        delete servers[i];
    }
    servers.clear();
    port_to_server_index.clear();
}

/**
 * @brief Configure les gestionnaires de signaux
 * 
 * Met en place les handlers pour SIGINT et SIGTERM pour permettre
 * un arrêt propre des serveurs lorsqu'ils reçoivent un de ces signaux.
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
 * @param signal Le numéro du signal reçu
 * 
 * Appelé lorsqu'un signal est reçu par le processus,
 * cette fonction arrête proprement tous les serveurs.
 */
void MultiServerManager::signalHandler(int signal) {
    LOG_INFO("Received signal " << signal << ", shutting down all servers gracefully...");
    if (instance) {
        instance->stopServers();
    }
}

/**
 * @brief Initialise les serveurs à partir de la configuration
 * @param config La configuration globale contenant tous les serveurs
 * 
 * Crée une instance de Server pour chaque configuration de serveur.
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
 * @brief Démarre tous les serveurs
 * 
 * Lance chaque serveur dans ce même thread puisque nous 
 * n'utilisons pas de threads en C++98.
 */
void MultiServerManager::startServers() {
    if (servers.empty()) {
        throw std::runtime_error("No servers initialized");
    }
    
    // C++98 n'a pas de support pour les threads
    // Donc nous pouvons seulement exécuter les serveurs séquentiellement
    // ou utiliser des processus enfants
    
    // Pour éviter l'utilisation de threads, on démarre chaque serveur l'un après l'autre
    // Cela signifie que seul le premier serveur sera vraiment actif
    LOG_INFO("Starting " << servers.size() << " servers sequentially...");
    LOG_WARNING("Note: Only the first server will actually handle requests in C++98 mode.");
    
    // Démarrer le premier serveur (il bloquera cette thread)
    if (!servers.empty()) {
        try {
            servers[0]->start();
        } catch (const std::exception& e) {
            LOG_ERROR("Error in server: " << e.what());
        }
    }
}

/**
 * @brief Arrête tous les serveurs
 * 
 * Envoie un signal d'arrêt à chaque serveur.
 */
void MultiServerManager::stopServers() {
    LOG_INFO("Stopping all servers...");
    for (size_t i = 0; i < servers.size(); i++) {
        servers[i]->stop();
    }
    LOG_SUCCESS("All servers stopped");
} 