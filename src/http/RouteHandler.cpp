#include "http/RouteHandler.hpp"
#include "http/HttpResponse.hpp"
#include "http/HttpRequest.hpp"
#include "http/utils/FileUtils.hpp"
#include "http/utils/HttpStringUtils.hpp"
#include "utils/Common.hpp"
#include "http/CGIHandler.hpp"
#include "http/parser/FormParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <ctime>
#include <algorithm>
#include <map>
#include <cerrno>   // For errno
#include <iomanip>  // For std::setprecision
#include "http/utils/HttpUtils.hpp"

HttpResponse RouteHandler::processRequest(const HttpRequest& request) {
    std::string file_path = getFilePath(request.getUri());
    
    if (request.getMethod() == "GET") {
        return handleGetRequest(request);
    }
    else if (request.getMethod() == "POST") {
        return handlePostRequest(request, file_path);
    }
    else if (request.getMethod() == "DELETE") {
        return handleDeleteRequest(request);
    }
    
    return HttpResponse::createError(405, "Method Not Allowed");
}

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

    // Si c'est une ressource CGI, la traiter comme telle
    if (isCgiResource(file_path)) {
        return handleCGIRequest(request, file_path);
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

HttpResponse RouteHandler::handlePostRequest(const HttpRequest& request, const std::string& file_path) {
    // Si c'est une requête pour l'API tasks
    if (request.getUri().find("/api/tasks") == 0) {
        return handleTasksApiRequest(request);
    }
    
    // Si c'est un upload de fichier
    if (request.getUri() == "/upload") {
        const std::string& content_type = request.getHeader("content-type");
        if (content_type.find("multipart/form-data") == std::string::npos) {
            return HttpResponse::createError(400, "Invalid Content-Type for file upload");
        }
        
        const std::map<std::string, UploadedFile>& uploaded_files = request.getFormData().getUploadedFiles();
        if (uploaded_files.empty()) {
            return HttpResponse::createError(400, "No files uploaded");
        }
        
        // Utiliser le chemin d'upload configuré
        std::string upload_dir = server_config.root_directory + "/uploads/";
        
        // Trouver la configuration de location correspondante
        std::map<std::string, LocationConfig>::const_iterator it = server_config.locations.find("/upload");
        if (it == server_config.locations.end()) {
            return HttpResponse::createError(500, "Upload location not configured");
        }
        
        UploadConfig upload_config(upload_dir, it->second.client_max_body_size);
        FileUploadHandler upload_handler(upload_config);
        
        // Traiter chaque fichier
        int success_count = 0;
        for (std::map<std::string, UploadedFile>::const_iterator it = uploaded_files.begin();
             it != uploaded_files.end(); ++it) {
            if (upload_handler.handleFileUpload(it->second)) {
                success_count++;
            }
        }
        
        return FileUploadHandler::createUploadResponse(success_count, uploaded_files.size());
    }
    
    // Si c'est une ressource CGI
    if (isCgiResource(file_path)) {
        return handleCGIRequest(request, file_path);
    }
    
    // Méthode POST non supportée pour cette ressource
    return HttpResponse::createError(405, "Method Not Allowed");
}

HttpResponse RouteHandler::handleDeleteRequest(const HttpRequest& request) {
    HttpResponse response;
    const std::string uri = request.getUri();
    
    // Vérifier si c'est une requête pour l'API des tâches
    if (uri.find("/api/tasks/") == 0) {
        return handleCGIRequest(request, getFilePath("/cgi-bin/tasks.php"));
    }
    
    // Pour les requêtes de suppression de fichier
    // Décoder l'URI avant de chercher le fichier
    std::string decoded_uri = HttpUtils::urlDecode(uri);
    std::string file_path = getFilePath(decoded_uri);
    
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

std::string RouteHandler::getFilePath(const std::string& uri) const {
    // Supprimer les paramètres de l'URL s'il y en a
    std::string clean_uri = uri;
    size_t question_mark = clean_uri.find('?');
    if (question_mark != std::string::npos) {
        clean_uri = clean_uri.substr(0, question_mark);
    }
    
    // Construire le chemin complet
    std::string path = root_directory;
    if (path[path.length() - 1] == '/' && clean_uri[0] == '/') {
        path = path.substr(0, path.length() - 1);
    }
    path += clean_uri;
    
    // Suppression des logs de debug
    // std::cerr << "[DEBUG] File Path: " << path << std::endl;
    
    return path;
}

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

    // Ajouter Content-Disposition: attachment pour les fichiers à télécharger
    // Pour forcer le téléchargement au lieu de l'affichage dans le navigateur
    bool is_upload_file = file_path.find("/uploads/") != std::string::npos;
    bool is_binary_type = mime_type.find("image/") == 0 || 
                          mime_type.find("application/") == 0 ||
                          mime_type.find("video/") == 0 ||
                          mime_type.find("audio/") == 0;

    if (is_upload_file || is_binary_type) {
        // Extraire le nom du fichier à partir du chemin
        size_t last_slash = file_path.find_last_of('/');
        std::string filename = (last_slash != std::string::npos) ? 
                               file_path.substr(last_slash + 1) : file_path;
        
        response.setHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    }

    return true;
}

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
    // Check if there's a custom error page for this error code
    std::map<int, std::string>::const_iterator it = server_config.error_pages.find(error_code);
    if (it != server_config.error_pages.end()) {
        // Use custom error page
        std::string error_page_path = root_directory + "/" + it->second;
        if (FileUtils::fileExists(error_page_path)) {
            HttpResponse response;
            response.setStatus(error_code);
            if (serveStaticFile(error_page_path, response)) {
                return response;
            }
            // If failed to serve the custom error page, fall back to default
        }
    }
    
    // Default error page generation if no custom page is available or failed to serve
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
    // Vérifier si l'extension est dans les handlers CGI configurés
    std::map<std::string, std::string>::const_iterator it = location.cgi_handlers.find(ext);
    return it != location.cgi_handlers.end();
}

HttpResponse RouteHandler::handleCGIRequest(const HttpRequest& request, const std::string& scriptPath) {
    std::string ext = getFileExtension(scriptPath);
    const LocationConfig* location = findMatchingLocation(request.getUri());
    if (!location) {
        return HttpResponse::createError(500, "No matching location for CGI request");
    }
    
    // Vérifier si l'extension est gérée par un handler CGI
    std::map<std::string, std::string>::const_iterator it = location->cgi_handlers.find(ext);
    if (it != location->cgi_handlers.end()) {
        // Construire le chemin absolu correctement
        std::string absolutePath = scriptPath;
        
        // Si le chemin ne commence pas par le répertoire racine, on l'ajoute
        if (absolutePath.find(root_directory) != 0) {
            if (absolutePath.substr(0, 2) == "./") {
                absolutePath = absolutePath.substr(2);
            }
            
            // Si le chemin commence par /, on supprime ce /
            if (absolutePath[0] == '/') {
                absolutePath = absolutePath.substr(1);
            }
            
            // Puis on préfixe avec le répertoire racine
            absolutePath = root_directory + "/" + absolutePath;
        }
        
        // Suppression des logs de debug
        // std::cerr << "[DEBUG] Script Path (before): " << scriptPath << std::endl;
        // std::cerr << "[DEBUG] Root Directory: " << root_directory << std::endl;
        // std::cerr << "[DEBUG] Absolute Path: " << absolutePath << std::endl;
        
        CGIHandler handler(request, absolutePath, it->second);
        return handler.executeCGI();
    }
    
    return HttpResponse::createError(500, "No CGI handler found for extension");
}

std::string RouteHandler::getFileExtension(const std::string& path) const {
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    return path.substr(dot_pos);
}

HttpResponse RouteHandler::handleTasksApiRequest(const HttpRequest& /* request */) {
    // Implémentation de l'API tasks
    return HttpResponse::createError(501, "Not Implemented");
}

bool RouteHandler::isCgiResource(const std::string& path) const {
    // Vérification simple des extensions
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string ext = path.substr(dot_pos);
        // Extensions CGI supportées
        return ext == ".cgi" || ext == ".php" || ext == ".py" || ext == ".pl";
    }
    return false;
}

std::string RouteHandler::getCGIInterpreter(const std::string& extension) const {
    static const std::map<std::string, std::string> interpreters = create_interpreter_map();
    
    std::map<std::string, std::string>::const_iterator it = interpreters.find(extension);
    if (it != interpreters.end()) {
        return it->second;
    }
    throw std::runtime_error("No interpreter found for extension: " + extension);
}

std::map<std::string, std::string> RouteHandler::create_interpreter_map() {
    std::map<std::string, std::string> m;
    m[".php"] = "/usr/bin/php-cgi";
    m[".py"] = "/usr/bin/python3";
    m[".pl"] = "/usr/bin/perl";
    m[".cgi"] = "/bin/sh";
    return m;
} 