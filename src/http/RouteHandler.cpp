#include "http/RouteHandler.hpp"
#include "http/utils/FileUtils.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

/**
 * @brief Constructeur
 * @param root_directory Le répertoire racine pour les fichiers statiques
 */
RouteHandler::RouteHandler(const std::string& root_directory)
    : root_directory(root_directory) {
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
    std::cout << "📥 Processing " << request.getMethod() << " request for: " << request.getUri() << std::endl;
    
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
    
    // Vérifier si le fichier existe
    if (!FileUtils::fileExists(file_path)) {
        return createErrorResponse(404, "Not Found");
    }

    // Vérifier si c'est un répertoire
    if (FileUtils::isDirectory(file_path)) {
        // Vérifier si un fichier index.html existe dans le répertoire
        std::string index_path = file_path;
        if (index_path[index_path.length() - 1] != '/') {
            index_path += '/';
        }
        index_path += "index.html";

        if (FileUtils::fileExists(index_path)) {
            // Servir le fichier index.html
            return serveStaticFile(index_path, response) ? 
                response : createErrorResponse(500, "Internal Server Error");
        } else {
            // Générer une liste du répertoire
            std::string uri = request.getUri();
            if (uri[uri.length() - 1] != '/') {
                // Rediriger pour ajouter un '/' à la fin de l'URI
                response.setRedirect(uri + "/", 301);
                return response;
            }
            
            std::string listing = FileUtils::generateDirectoryListing(file_path, request.getUri());
            response.setBody(listing, "text/html");
            return response;
        }
    }

    // Vérifier les permissions de lecture
    if (!FileUtils::hasReadPermission(file_path)) {
        return createErrorResponse(403, "Forbidden");
    }

    // Servir le fichier
    return serveStaticFile(file_path, response) ? 
        response : createErrorResponse(500, "Internal Server Error");
}

/**
 * @brief Traite une requête POST
 * @param request La requête HTTP
 * @return La réponse HTTP
 */
HttpResponse RouteHandler::handlePostRequest(const HttpRequest& request) {
    // Utiliser le paramètre pour éviter l'avertissement de paramètre non utilisé
    std::cout << "Requête POST reçue pour: " << request.getUri() << std::endl;
    
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
    // Utiliser le paramètre pour éviter l'avertissement de paramètre non utilisé
    std::cout << "Requête DELETE reçue pour: " << request.getUri() << std::endl;
    
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

    // Définir le corps de la réponse avec le bon type MIME
    response.setBody(content, mime_type);

    // Ajouter des headers pour le cache
    response.setHeader("ETag", calculateETag(file_path));
    response.setHeader("Cache-Control", "max-age=3600"); // Cache d'une heure

    return true;
} 