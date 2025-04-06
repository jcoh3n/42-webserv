#include "http/RouteHandler.hpp"
#include "http/HttpResponse.hpp"
#include "http/utils/FileUtils.hpp"
#include "http/utils/HttpStringUtils.hpp"
#include "utils/Common.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <ctime>
#include <algorithm>
#include <map>

/**
 * @brief Constructeur
 * @param root_directory Le répertoire racine pour les fichiers statiques
 * @param server_config La configuration du serveur
 */
RouteHandler::RouteHandler(const std::string& root_directory, const ServerConfig& server_config)
    : root_directory(root_directory)
    , server_config(server_config) {
}

/**
 * @brief Destructeur
 */
RouteHandler::~RouteHandler() {
}

/**
 * @brief Traite une requête HTTP et génère une réponse appropriée
 * @param request La requête HTTP à traiter
 * @return La réponse HTTP à envoyer au client
 */
HttpResponse RouteHandler::processRequest(const HttpRequest& request) {
    // Trouver la location correspondante
    const LocationConfig* location = findMatchingLocation(request.getUri());
    if (!location) {
        return serveErrorPage(404, "Location not found");
    }

    // Vérifier si la méthode est autorisée
    if (std::find(location->allowed_methods.begin(), 
                  location->allowed_methods.end(), 
                  request.getMethod()) == location->allowed_methods.end()) {
        return serveErrorPage(405, "Method not allowed");
    }

    // Construire le chemin du fichier
    std::string file_path = getFilePath(request.getUri());

    // Vérifier si c'est une requête CGI
    if (isCGIRequest(file_path, *location)) {
        return handleCGIRequest(request, file_path, *location);
    }

    // Traiter comme une requête normale
    if (request.getMethod() == "GET") {
        return handleGetRequest(request);
    } else if (request.getMethod() == "POST") {
        return handlePostRequest(request);
    } else if (request.getMethod() == "DELETE") {
        return handleDeleteRequest(request);
    }

    return serveErrorPage(501, "Method not implemented");
}

/**
 * @brief Traite une requête GET
 * @param request La requête HTTP
 * @return La réponse HTTP
 */
HttpResponse RouteHandler::handleGetRequest(const HttpRequest& request) {
    HttpResponse response;
    std::string file_path = getFilePath(request.getUri()); 
    
    // Vérifier d'abord si le fichier existe | Cas: 404
    if (!FileUtils::fileExists(file_path)) {
        return serveErrorPage(404, "Not Found");
    }

    // Ensuite vérifier les permissions de lecture | Cas: 403
    if (!FileUtils::hasReadPermission(file_path)) {
        return serveErrorPage(403, "Forbidden");
    }

    // Traitement des répertoires | Cas: 301 -> Rediriger vers le répertoire
    if (FileUtils::isDirectory(file_path)) {
        std::string uri = request.getUri();
        // Rediriger si l'URI ne se termine pas par '/'
        if (uri[uri.length() - 1] != '/') {
            response.setRedirect(uri + "/", 301);
            return response;
        }

        // Chercher index.html
        std::string index_path = file_path + "/index.html";
        if (FileUtils::fileExists(index_path)) {
            // Vérifier la validation du cache pour index.html
            if (checkNotModified(request, index_path, response)) {
                return response;
            }
            return serveStaticFile(index_path, response) ? 
                response : serveErrorPage(500, "Internal Server Error");
        }
        
        // Générer la liste du répertoire
        std::string listing = FileUtils::generateDirectoryListing(file_path, uri);
        response.setBody(listing, "text/html");
        return response;
    }

    // Vérifier la validation du cache avant de servir le fichier
    if (checkNotModified(request, file_path, response)) {
        return response;
    }

    // Servir le fichier statique | Cas: 500 -> Erreur interne du serveur
    if (!serveStaticFile(file_path, response)) {
        return serveErrorPage(500, "Internal Server Error");
    }

    return response;
}

/**
 * @brief Traite une requête POST
 * @param request La requête HTTP
 * @return La réponse HTTP
 */
HttpResponse RouteHandler::handlePostRequest(const HttpRequest& request) {
    HttpResponse response;
    const std::string uri = request.getUri();
    
    // Vérifier si c'est une requête pour l'API des tâches
    if (uri == "/api/tasks") {
        return handleCGIRequest(request, getFilePath("/cgi-bin/tasks.php"), *findMatchingLocation("/cgi-bin/"));
    }
    
    // Vérifier si c'est un formulaire ou une autre ressource CGI
    const LocationConfig* location = findMatchingLocation(uri);
    if (location && !location->cgi_handlers.empty()) {
        std::string file_path = getFilePath(uri);
        return handleCGIRequest(request, file_path, *location);
    }
    
    // Vérifier si c'est une requête de téléchargement de fichier
    if (request.getHeader("content-type").find("multipart/form-data") != std::string::npos) {
        // Logique pour traiter les uploads de fichiers
        // (Cette partie serait implémentée selon le breakdown file_upload.md)
        return HttpResponse::createError(501, "File upload not yet implemented");
    }
    
    // Pour les autres types de requêtes POST
    // Si aucun handler spécifique n'est trouvé, renvoyer une erreur
    return HttpResponse::createError(400, "Invalid POST request");
}

/**
 * @brief Traite une requête DELETE
 * @param request La requête HTTP
 * @return La réponse HTTP
 */
HttpResponse RouteHandler::handleDeleteRequest(const HttpRequest& request) {
    HttpResponse response;
    const std::string uri = request.getUri();
    
    // Vérifier si c'est une requête pour l'API des tâches
    if (uri.find("/api/tasks/") == 0) {
        return handleCGIRequest(request, getFilePath("/cgi-bin/tasks.php"), *findMatchingLocation("/cgi-bin/"));
    }
    
    // Pour les requêtes de suppression de fichier
    std::string file_path = getFilePath(uri);
    
    // Vérifier si le fichier existe
    if (!FileUtils::fileExists(file_path)) {
        return serveErrorPage(404, "File not found");
    }
    
    // Vérifier les permissions
    if (!FileUtils::hasWritePermission(file_path)) {
        return serveErrorPage(403, "Forbidden: No permission to delete file");
    }
    
    // Essayer de supprimer le fichier
    if (std::remove(file_path.c_str()) != 0) {
        return serveErrorPage(500, "Failed to delete file");
    }
    
    // Réponse réussie (No Content)
    response.setStatus(204);
    return response;
}

/**
 * @brief Obtient le chemin du fichier correspondant à l'URI
 * @param uri L'URI de la requête
 * @return Le chemin complet du fichier
 */
std::string RouteHandler::getFilePath(const std::string& uri) {
    // Normaliser le chemin pour éviter les attaques par traversée de répertoire
    return FileUtils::normalizePath(root_directory, uri);
}

/**
 * @brief Lit un fichier et le place dans la réponse HTTP
 * @param file_path Le chemin du fichier à servir
 * @param response La réponse HTTP à compléter
 * @return true si le fichier a été servi avec succès, false sinon
 */
bool RouteHandler::serveStaticFile(const std::string& file_path, HttpResponse& response) {
    // Ouvrir le fichier
    std::ifstream file(file_path.c_str(), std::ios::binary); 
    if (!file) {
        return false;
    }

    // Lire le contenu du fichier
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Définir le type MIME
    std::string mime_type = getMimeType(file_path);

    // Définir le body de la réponse avec le bon type MIME
    response.setBody(content, mime_type);

    // Ajouter l'ETag pour la validation du cache
    response.setHeader("ETag", calculateETag(file_path));
    
    // Configuration simple du Cache-Control
    // Les fichiers statiques sont mis en cache pendant 1 heure par défaut
    response.setHeader("Cache-Control", "public, max-age=3600");
    
    // Ajouter la date de dernière modification
    struct stat file_stat;
    if (stat(file_path.c_str(), &file_stat) == 0) {
        char last_modified[100];
        struct tm* tm_info = gmtime(&file_stat.st_mtime);
        strftime(last_modified, sizeof(last_modified), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
        response.setHeader("Last-Modified", last_modified);
    }

    return true;
}

/**
 * @brief Vérifie si une ressource a été modifiée depuis la dernière requête
 * @param request La requête HTTP
 * @param file_path Le chemin du fichier
 * @param response La réponse HTTP à compléter
 * @return true si la ressource n'a pas été modifiée, false sinon
 */
bool RouteHandler::checkNotModified(const HttpRequest& request, const std::string& file_path, HttpResponse& response) {
    // Vérifier si le client a envoyé un ETag
    std::string if_none_match = request.getHeader("If-None-Match");
    
    if (!if_none_match.empty()) {
        // Calculer l'ETag de la ressource
        std::string etag = calculateETag(file_path);
        
        // Normaliser les ETags pour la comparaison (enlever les guillemets)
        std::string normalized_client_etag = HttpStringUtils::normalizeETag(if_none_match);
        std::string normalized_server_etag = HttpStringUtils::normalizeETag(etag);
        
        // Vérifier si l'ETag correspond après normalisation
        if (normalized_client_etag == normalized_server_etag) {
            // La ressource n'a pas été modifiée
            response.setNotModified(etag);
            return true;
        }
    }
    
    // Vérifier si le client a envoyé une date de dernière modification
    std::string if_modified_since = request.getHeader("If-Modified-Since");
    if (!if_modified_since.empty()) {
        // Obtenir la date de dernière modification du fichier
        struct stat file_stat;
        if (stat(file_path.c_str(), &file_stat) == 0) {
            // Convertir la date du fichier en format HTTP
            struct tm* tm_info = gmtime(&file_stat.st_mtime);
            char last_modified[100];
            strftime(last_modified, sizeof(last_modified), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
            
            // Vérifier si la date correspond
            if (if_modified_since == last_modified) {
                // La ressource n'a pas été modifiée
                response.setStatus(304);
                response.setHeader("Last-Modified", last_modified);
                return true;
            }
        }
    }
    
    // La ressource a été modifiée ou pas d'information de cache
    return false;
}

HttpResponse RouteHandler::serveErrorPage(int error_code, const std::string& message) {
    return HttpResponse::createError(error_code, message);
}

const LocationConfig* RouteHandler::findMatchingLocation(const std::string& uri) const {
    const LocationConfig* best_location = NULL;
    size_t best_match_length = 0;
    
    std::map<std::string, LocationConfig>::const_iterator it;
    for (it = server_config.locations.begin(); it != server_config.locations.end(); ++it) {
        const std::string& location_path = it->first;
        if (uri.find(location_path) == 0) {
            if (location_path.length() > best_match_length) {
                best_match_length = location_path.length();
                best_location = &it->second;
            }
        }
    }
    
    return best_location;
}

bool RouteHandler::isCGIRequest(const std::string& path, const LocationConfig& location) const {
    std::string ext = getFileExtension(path);
    std::map<std::string, std::string>::const_iterator it = location.cgi_handlers.find(ext);
    return it != location.cgi_handlers.end();
}

HttpResponse RouteHandler::handleCGIRequest(const HttpRequest& request, 
                                          const std::string& script_path, 
                                          const LocationConfig& location) {
    std::string ext = getFileExtension(script_path);
    std::map<std::string, std::string>::const_iterator it = location.cgi_handlers.find(ext);
    if (it == location.cgi_handlers.end()) {
        return HttpResponse::createError(500, "No CGI handler found for extension");
    }

    CGIHandler handler(request, script_path, it->second);
    return handler.executeCGI();
}

/**
 * @brief Obtient l'extension d'un fichier
 * @param path Le chemin du fichier
 * @return L'extension du fichier (avec le point)
 */
std::string RouteHandler::getFileExtension(const std::string& path) const {
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    return path.substr(dot_pos);
}
