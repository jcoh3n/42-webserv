#ifndef ROUTE_HANDLER_HPP
#define ROUTE_HANDLER_HPP

#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/utils/FileUtils.hpp"
#include <string>

/**
 * @brief Classe pour traiter les requêtes HTTP et générer les réponses appropriées
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
    
    // Méthodes de gestion du cache
    bool checkNotModified(const HttpRequest& request, const std::string& file_path, HttpResponse& response);
};

#endif // ROUTE_HANDLER_HPP 