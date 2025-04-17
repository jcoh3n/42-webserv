#ifndef ROUTE_HANDLER_HPP
#define ROUTE_HANDLER_HPP

#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/upload/FileUploadHandler.hpp"
#include "http/upload/UploadConfig.hpp"
#include "config/ConfigTypes.hpp"
#include <string>
#include <map>

/**
 * @brief Classe pour traiter les requêtes HTTP et générer les réponses appropriées
 */
class RouteHandler {
public:
    // Constructeur et destructeur
    RouteHandler(const std::string& root, const ServerConfig& config)
        : root_directory(root)
        , server_config(config) {}
    virtual ~RouteHandler() {}

    // Méthode principale pour traiter les requêtes
    HttpResponse processRequest(const HttpRequest& request);
    
    // Méthode pour trouver la location correspondante à une URI
    const LocationConfig* findMatchingLocation(const std::string& uri) const;

private:
    // Répertoire racine pour les fichiers statiques
    std::string root_directory;
    // Configuration du serveur
    const ServerConfig& server_config;

    // Méthodes de traitement par type de requête
    HttpResponse handleGetRequest(const HttpRequest& request);
    /**
     * @brief Traite une requête POST
     * 
     * @param request La requête HTTP
     * @param file_path Le chemin du fichier cible
     * @return La réponse HTTP
     */
    HttpResponse handlePostRequest(const HttpRequest& request, const std::string& file_path);
    HttpResponse handleDeleteRequest(const HttpRequest& request);
    HttpResponse handleCGIRequest(const HttpRequest& request, const std::string& scriptPath);
    HttpResponse handleTasksApiRequest(const HttpRequest& request);
    
    // Méthodes utilitaires
    bool isCgiResource(const std::string& path) const;
    std::string getFilePath(const std::string& uri) const;
    bool serveStaticFile(const std::string& file_path, HttpResponse& response);
    bool isCGIRequest(const std::string& path, const LocationConfig& location) const;
    std::string getFileExtension(const std::string& path) const;
    
    // Méthodes de gestion du cache
    bool checkNotModified(const HttpRequest& request, const std::string& file_path, HttpResponse& response);
    HttpResponse serveErrorPage(int error_code, const std::string& message);
};

#endif // ROUTE_HANDLER_HPP 