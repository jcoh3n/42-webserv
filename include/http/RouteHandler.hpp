#ifndef ROUTE_HANDLER_HPP
#define ROUTE_HANDLER_HPP

#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/utils/FileUtils.hpp"
#include <string>

/**
 * @brief Classe pour gérer les routes et traiter les requêtes HTTP
 */
class RouteHandler {
public:
    // Constructeur et destructeur
    RouteHandler(const std::string& root_directory = "./www");
    ~RouteHandler();

    // Méthode principale pour traiter une requête
    HttpResponse processRequest(const HttpRequest& request);

private:
    std::string root_directory;  // Répertoire racine pour les fichiers statiques

    // Méthodes pour traiter différentes méthodes HTTP
    HttpResponse handleGetRequest(const HttpRequest& request);
    HttpResponse handlePostRequest(const HttpRequest& request);
    HttpResponse handleDeleteRequest(const HttpRequest& request);
    
    // Méthodes utilitaires
    std::string getFilePath(const std::string& uri);
    bool serveStaticFile(const std::string& file_path, HttpResponse& response);
};

#endif // ROUTE_HANDLER_HPP 