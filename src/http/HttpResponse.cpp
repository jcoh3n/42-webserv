#include "http/HttpResponse.hpp"
#include "http/HttpRequest.hpp"
#include "http/utils/HttpStringUtils.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <ctime>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <cctype>

/**
 * @brief Convertit une valeur numérique en chaîne de caractères
 * @tparam T Type de la valeur numérique
 * @param value La valeur à convertir
 * @return La chaîne de caractères correspondante
 */
template<typename T>
static std::string numberToString(T value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

/**
 * @brief Constructeur par défaut
 * 
 * Initialise une réponse HTTP avec un code 200 (OK)
 * et définit les en-têtes de base.
 */
HttpResponse::HttpResponse() : status_code(200), status_message("OK") {
    setHeader("Server", "webserv/1.0");
    setHeader("Connection", "keep-alive");
}

/**
 * @brief Destructeur
 */
HttpResponse::~HttpResponse() {
}

/**
 * @brief Définit le code de statut et le message de la réponse
 * @param code Le code de statut HTTP (ex: 200, 404, 500)
 * @param message Le message de statut (optionnel)
 * 
 * Si le message est vide, un message par défaut correspondant
 * au code de statut sera utilisé.
 */
void HttpResponse::setStatus(int code, const std::string& message) {
    status_code = code;
    if (!message.empty()) {
        status_message = message;
    } else {
        // Messages par défaut
        switch(code) {
            case 100: status_message = "Continue"; break;
            case 101: status_message = "Switching Protocols"; break;
            case 200: status_message = "OK"; break;
            case 201: status_message = "Created"; break;
            case 202: status_message = "Accepted"; break;
            case 204: status_message = "No Content"; break;
            case 206: status_message = "Partial Content"; break;
            case 300: status_message = "Multiple Choices"; break;
            case 301: status_message = "Moved Permanently"; break;
            case 302: status_message = "Found"; break;
            case 303: status_message = "See Other"; break;
            case 304: status_message = "Not Modified"; break;
            case 307: status_message = "Temporary Redirect"; break;
            case 308: status_message = "Permanent Redirect"; break;
            case 400: status_message = "Bad Request"; break;
            case 401: status_message = "Unauthorized"; break;
            case 403: status_message = "Forbidden"; break;
            case 404: status_message = "Not Found"; break;
            case 405: status_message = "Method Not Allowed"; break;
            case 406: status_message = "Not Acceptable"; break;
            case 408: status_message = "Request Timeout"; break;
            case 409: status_message = "Conflict"; break;
            case 413: status_message = "Payload Too Large"; break;
            case 414: status_message = "URI Too Long"; break;
            case 415: status_message = "Unsupported Media Type"; break;
            case 429: status_message = "Too Many Requests"; break;
            case 500: status_message = "Internal Server Error"; break;
            case 501: status_message = "Not Implemented"; break;
            case 502: status_message = "Bad Gateway"; break;
            case 503: status_message = "Service Unavailable"; break;
            case 504: status_message = "Gateway Timeout"; break;
            default: status_message = "Unknown Status"; break;
        }
    }
}

/**
 * @brief Définit un en-tête HTTP
 * @param key Le nom de l'en-tête
 * @param value La valeur de l'en-tête
 */
void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers[key] = value;
}

/**
 * @brief Définit le body de la réponse et met à jour les en-têtes associés
 * @param content Le contenu du body
 * @param content_type Le type MIME du contenu
 * 
 * Met à jour les en-têtes Content-Type et Content-Length.
 */
void HttpResponse::setBody(const std::string& content, const std::string& content_type) {
    body = content;
    setHeader("Content-Type", content_type);
    setHeader("Content-Length", numberToString(body.size()));
}

/**
 * @brief Configure une réponse 304 Not Modified
 * @param etag La valeur de l'ETag pour la validation du cache
 * 
 * Utilisé pour indiquer au client que la ressource n'a pas été modifiée
 * depuis la dernière requête.
 */
void HttpResponse::setNotModified(const std::string& etag) {
    setStatus(304);
    body.clear();
    setHeader("ETag", etag);
    setHeader("Content-Length", "0");
}

/**
 * @brief Configure une réponse de redirection
 * @param location L'URL de destination
 * @param code Le code de statut à utiliser (301, 302, 303, etc.)
 * 
 * Configure une redirection HTTP avec un body HTML expliquant
 * la redirection pour les clients qui ne suivent pas automatiquement.
 */
void HttpResponse::setRedirect(const std::string& location, int code) {
    setStatus(code);
    body.clear();
    setHeader("Location", location);
    
    // Message HTML pour les navigateurs qui ne suivent pas automatiquement les redirections
    std::string redirect_body = "<html><head><title>Redirect</title></head><body>"
                               "<h1>" + numberToString(code) + " " + status_message + "</h1>"
                               "<p>The document has moved <a href=\"" + location + "\">here</a>.</p>"
                               "</body></html>";
    setBody(redirect_body);
}

/**
 * @brief Active le transfert en mode chunked
 * 
 * Supprime l'en-tête Content-Length et définit Transfer-Encoding: chunked.
 * Utilisé pour les réponses de taille inconnue ou dynamique.
 */
void HttpResponse::setChunkedTransfer() {
    headers.erase("Content-Length");
    setHeader("Transfer-Encoding", "chunked");
}

/**
 * @brief Active le Cross-Origin Resource Sharing (CORS)
 * @param origin L'origine autorisée
 * @param methods Les méthodes HTTP autorisées
 * @param headers_allowed Les en-têtes HTTP autorisés
 * 
 * Configure les en-têtes CORS pour autoriser les requêtes
 * cross-origin selon les paramètres spécifiés.
 */
void HttpResponse::enableCORS(const std::string& origin, 
                const std::string& methods, 
                const std::string& headers_allowed) {
    setHeader("Access-Control-Allow-Origin", origin);
    setHeader("Access-Control-Allow-Methods", methods);
    if (!headers_allowed.empty()) {
        setHeader("Access-Control-Allow-Headers", headers_allowed);
    }
}

/**
 * @brief Construit la réponse HTTP complète
 * @return La chaîne de caractères représentant la réponse HTTP
 * 
 * Génère la réponse HTTP complète, incluant la ligne de statut,
 * les en-têtes et le body.
 */
std::string HttpResponse::build() const {
    std::stringstream response;
    
    // Ligne de statut
    response << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";
    
    // Headers
    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }
    
    // Séparation headers/body
    response << "\r\n";
    
    // Body
    if (!body.empty()) {
        response << body;
    }
    
    return response.str();
}

/**
 * @brief Construit une réponse pour la méthode HEAD (sans body)
 * @return La chaîne de caractères représentant la réponse HTTP sans le body
 * 
 * Similaire à build() mais n'inclut pas le body de la réponse.
 * Utilisé pour répondre aux requêtes HEAD.
 */
std::string HttpResponse::buildHeadResponse() const {
    std::stringstream response;
    
    // Ligne de statut
    response << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";
    
    // Headers
    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }
    
    // Séparation headers/body sans inclure le body
    response << "\r\n";
    
    return response.str();
}

/**
 * @brief Crée une réponse d'erreur HTTP
 * @param error_code Le code d'erreur HTTP (ex: 404, 500)
 * @param message Le message d'erreur (optionnel)
 * @return Une réponse HTTP formatée pour l'erreur
 * 
 * Génère une réponse d'erreur HTTP avec un corps HTML simple
 * décrivant l'erreur.
 */
HttpResponse createErrorResponse(int error_code, const std::string& message) {
    HttpResponse response;
    response.setStatus(error_code);
    
    std::string error_message = message.empty() ? response.getStatusMessage() : message;
    std::string error_page = "<html><body><h1>" + 
                             numberToString(error_code) + " " + 
                             error_message + 
                             "</h1></body></html>";
    
    response.setBody(error_page);
    return response;
}

/**
 * @brief Détermine le type MIME d'un fichier
 * @param file_path Le chemin du fichier
 * @return Le type MIME correspondant
 * 
 * Analyse l'extension du fichier pour déterminer son type MIME.
 * Retourne "application/octet-stream" si le type est inconnu.
 */
std::string getMimeType(const std::string& file_path) {
    static std::map<std::string, std::string> mime_types;
    
    // Initialisation si nécessaire (première fois seulement)
    if (mime_types.empty()) {
        mime_types[".html"] = "text/html";
        mime_types[".htm"] = "text/html";
        mime_types[".css"] = "text/css";
        mime_types[".js"] = "application/javascript";
        mime_types[".json"] = "application/json";
        mime_types[".xml"] = "application/xml";
        mime_types[".txt"] = "text/plain";
        mime_types[".jpg"] = "image/jpeg";
        mime_types[".jpeg"] = "image/jpeg";
        mime_types[".png"] = "image/png";
        mime_types[".gif"] = "image/gif";
        mime_types[".svg"] = "image/svg+xml";
        mime_types[".ico"] = "image/x-icon";
        mime_types[".pdf"] = "application/pdf";
        mime_types[".zip"] = "application/zip";
        mime_types[".gz"] = "application/gzip";
        mime_types[".tar"] = "application/x-tar";
        mime_types[".mp3"] = "audio/mpeg";
        mime_types[".mp4"] = "video/mp4";
        mime_types[".mpeg"] = "video/mpeg";
        mime_types[".avi"] = "video/x-msvideo";
        mime_types[".webm"] = "video/webm";
        mime_types[".wav"] = "audio/wav";
        mime_types[".ogg"] = "audio/ogg";
        mime_types[".woff"] = "font/woff";
        mime_types[".woff2"] = "font/woff2";
        mime_types[".ttf"] = "font/ttf";
        mime_types[".otf"] = "font/otf";
        mime_types[".eot"] = "application/vnd.ms-fontobject";
        mime_types[".php"] = "text/html"; // PHP sera traité par CGI
    }

    // Trouver la position du '.' dans le chemin du fichier
    size_t dot_pos = file_path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "application/octet-stream"; // Type par défaut
    }

    std::string extension = file_path.substr(dot_pos); // Extraire l'extension du fichier
    std::map<std::string, std::string>::const_iterator it = mime_types.find(extension); // Parcourir map pour trouver le type MIM
    return it != mime_types.end() ? it->second : "application/octet-stream"; // Retourner le type MIME correspondant
}

/**
 * @brief Calcule l'ETag d'un fichier
 * @param file_path Le chemin du fichier
 * @return La valeur de l'ETag
 * 
 * Génère un identifiant unique pour le fichier basé sur sa taille
 * et sa date de dernière modification, utilisé pour la validation du cache.
 */
std::string calculateETag(const std::string& file_path) {
    struct stat file_info;
    if (stat(file_path.c_str(), &file_info) == 0) {
        // Calculer un ETag simple basé sur la taille et la date de modification
        // Ce format est suffisant pour la plupart des cas d'utilisation
        std::stringstream etag;
        etag << "\"" << file_info.st_size << "-" << file_info.st_mtime << "\"";
        return etag.str();
    }
    return "\"0\"";  // Même valeur par défaut que dans RouteHandler
}

/**
 * @brief Vérifie si une réponse 304 Not Modified peut être envoyée
 * @param request La requête HTTP
 * @param file_path Le chemin du fichier
 * @param response La réponse HTTP à modifier
 * @return true si une réponse 304 doit être envoyée, false sinon
 * 
 * Compare l'ETag du fichier avec celui dans la requête.
 * Si correspondance, configure la réponse comme 304 Not Modified.
 */
bool checkNotModified(const HttpRequest& request, const std::string& file_path, HttpResponse& response) {
    // Calculer l'ETag du fichier
    std::string etag = calculateETag(file_path);
    // Ne pas continuer si l'ETag est la valeur par défaut ("0")
    if (etag == "\"0\"") {
        return false;
    }
    
    // Vérifier les en-têtes de la requête
    const std::map<std::string, std::string>& headers = request.getHeaders();
    std::map<std::string, std::string>::const_iterator it = headers.find("if-none-match");
    
    // Si l'ETag correspondant a été trouvé
    if (it != headers.end()) {
        // Normaliser les ETags pour la comparaison
        std::string normalized_client_etag = HttpStringUtils::normalizeETag(it->second);
        std::string normalized_server_etag = HttpStringUtils::normalizeETag(etag);
        
        // Si l'ETag correspond après normalisation, répondre avec 304 Not Modified
        if (normalized_client_etag == normalized_server_etag) {
            response.setNotModified(etag);
            return true;
        }
    }
    
    // Sinon, ajouter l'ETag à la réponse et continuer normalement
    response.setHeader("ETag", etag);
    
    // Ajouter aussi Last-Modified pour compatibilité
    struct stat file_stat;
    if (stat(file_path.c_str(), &file_stat) == 0) {
        char last_modified[100];
        struct tm* tm_info = gmtime(&file_stat.st_mtime);
        strftime(last_modified, sizeof(last_modified), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
        response.setHeader("Last-Modified", last_modified);
    }
    
    return false;
} 