#ifndef CONFIG_TYPES_HPP
#define CONFIG_TYPES_HPP

#include <string>
#include <vector>
#include <map>

namespace HTTP {

/**
 * @brief Configuration d'une location (route) dans le serveur
 */
struct LocationConfig {
    std::vector<std::string> allowed_methods;  // Méthodes HTTP autorisées
    bool autoindex;                            // Activation de l'autoindex
    std::vector<std::string> index_files;      // Fichiers index par défaut
    std::string redirect_url;                  // URL de redirection (pour return)
    int redirect_code;                         // Code de redirection HTTP
    std::string root_override;                 // Répertoire racine spécifique à cette location
    std::vector<std::string> cgi_extensions;   // Extensions pour le CGI (ex: ".php")
    std::string upload_directory;              // Répertoire pour les uploads
    std::string alias;                         // Alias pour cette location
    
    LocationConfig() 
        : autoindex(false)
        , redirect_code(301) {}
};

/**
 * @brief Configuration d'un serveur virtuel
 */
struct ServerConfig {
    std::string host;                                // Adresse IP d'écoute
    int port;                                        // Port d'écoute
    std::vector<std::string> server_names;           // Noms de serveur (pour virtual hosting)
    std::string root_directory;                      // Répertoire racine pour ce serveur
    std::vector<std::string> index_files;            // Fichiers index par défaut
    std::map<int, std::string> error_pages;          // Pages d'erreur personnalisées
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

} // namespace HTTP

#endif // CONFIG_TYPES_HPP 