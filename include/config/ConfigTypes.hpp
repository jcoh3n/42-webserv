#ifndef CONFIG_TYPES_HPP
#define CONFIG_TYPES_HPP

#include <string>
#include <vector>
#include <map>

/**
 * @brief Configuration d'une location (route) dans le serveur
 */
struct LocationConfig {
    std::string path;                        // Chemin de la location (ex: "/api")
    std::vector<std::string> allowed_methods; // Méthodes HTTP autorisées
    bool directory_listing;                  // Activation de l'autoindex
    std::string index_file;                  // Fichier index par défaut
    std::string redirect_url;                // URL de redirection (pour return)
    std::string root_override;               // Répertoire racine spécifique à cette location
    std::string cgi_extension;               // Extension pour le CGI (ex: ".php")
    std::string upload_directory;            // Répertoire pour les uploads
    
    LocationConfig() 
        : directory_listing(false)
        , index_file("index.html") {}
};

/**
 * @brief Configuration d'un serveur virtuel
 */
struct ServerConfig {
    std::string host;                                // Adresse IP d'écoute
    int port;                                        // Port d'écoute
    std::vector<std::string> server_names;           // Noms de serveur (pour virtual hosting)
    std::string root_directory;                      // Répertoire racine pour ce serveur
    std::map<std::string, std::string> error_pages;  // Pages d'erreur personnalisées
    size_t client_max_body_size;                     // Taille maximale du body client
    std::map<std::string, LocationConfig> locations; // Configurations des locations
    
    ServerConfig() 
        : host("0.0.0.0")
        , port(8080)
        , root_directory("./www")
        , client_max_body_size(1024 * 1024) {} // 1MB par défaut
};

/**
 * @brief Configuration globale du webserv
 */
struct WebservConfig {
    std::vector<ServerConfig> servers; // Liste des serveurs configurés
};

#endif // CONFIG_TYPES_HPP 