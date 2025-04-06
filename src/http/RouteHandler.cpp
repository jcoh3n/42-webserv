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
    // Vérifier la méthode HTTP et rediriger vers le gestionnaire approprié
    if (request.getMethod() == "GET") {
        return handleGetRequest(request);
    } else if (request.getMethod() == "POST") {
        return handlePostRequest(request);
    } else if (request.getMethod() == "DELETE") {
        return handleDeleteRequest(request);
    } else {
        // Méthode non supportée
        return createErrorResponse(405, "Method Not Allowed");
    }
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
    // Éviter l'avertissement de paramètre non utilisé
    UNUSED(request);
    
    // Dans cette première version, nous ne gérons pas les POST
    // Cela sera implémenté dans une phase ultérieure
    return createErrorResponse(501, "Not Implemented");
}

/**
 * @brief Traite une requête DELETE
 * @param request La requête HTTP
 * @return La réponse HTTP
 */
HttpResponse RouteHandler::handleDeleteRequest(const HttpRequest& request) {
    // Éviter l'avertissement de paramètre non utilisé
    UNUSED(request);
    
    // Dans cette première version, nous ne gérons pas les DELETE
    // Cela sera implémenté dans une phase ultérieure
    return createErrorResponse(501, "Not Implemented");
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
    // Chercher la page d'erreur personnalisée dans la configuration
    std::map<int, std::string>::const_iterator it = server_config.error_pages.find(error_code);
    if (it != server_config.error_pages.end()) {
        std::string error_page_path = root_directory + "/" + it->second;
        if (FileUtils::fileExists(error_page_path) && FileUtils::hasReadPermission(error_page_path)) {
            HttpResponse response;
            response.setStatus(error_code);
            if (serveStaticFile(error_page_path, response)) {
                return response;
            }
        }
    }
    
    // Si la page personnalisée n'existe pas ou ne peut pas être servie, utiliser la page par défaut
    return createErrorResponse(error_code, message);
}
