#ifndef ROUTE_HANDLER_HPP
#define ROUTE_HANDLER_HPP

#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/utils/FileUtils.hpp"
#include "config/ConfigTypes.hpp"
#include <string>

/**
 * @brief Classe pour traiter les requêtes HTTP et générer les réponses appropriées
 */
class RouteHandler {
public:
    // Constructeur et destructeur
    RouteHandler(const std::string& root_directory, const ServerConfig& server_config);
    ~RouteHandler();

    // Méthode principale pour traiter les requêtes
    HttpResponse processRequest(const HttpRequest& request);

private:
    // Répertoire racine pour les fichiers statiques
    std::string root_directory;
    // Configuration du serveur
    const ServerConfig& server_config;

    // Méthodes de traitement par type de requête
    HttpResponse handleGetRequest(const HttpRequest& request);
    HttpResponse handlePostRequest(const HttpRequest& request);
    HttpResponse handleDeleteRequest(const HttpRequest& request);
    
    // Méthodes utilitaires
    std::string getFilePath(const std::string& uri);
    bool serveStaticFile(const std::string& file_path, HttpResponse& response);
    
    // Méthodes de gestion du cache
    bool checkNotModified(const HttpRequest& request, const std::string& file_path, HttpResponse& response);
    HttpResponse serveErrorPage(int error_code, const std::string& message);
};

#endif // ROUTE_HANDLER_HPP 