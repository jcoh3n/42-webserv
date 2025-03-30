## ÉTAPE 4 - CONSTRUCTION DES RÉPONSES HTTP

### Objectif
Implémenter un système pour générer des réponses HTTP valides selon le standard HTTP/1.1, incluant tous les types de contenus et les codes de statut appropriés.

### Structure de base d'une réponse HTTP
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 13\r\n
Connection: keep-alive\r\n
\r\n
Hello World!
Copier
### Détails techniques

1. **Classe HttpResponse**:
```cpp
class HttpResponse {
public:
    int status_code;
    std::string status_message;
    std::map<std::string, std::string> headers;
    std::string body;
    
    HttpResponse() : status_code(200), status_message("OK") {
        setHeader("Server", "webserv/1.0");
        setHeader("Connection", "keep-alive");
    }

    void setStatus(int code, const std::string& message = "") {
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

    void setHeader(const std::string& key, const std::string& value) {
        headers[key] = value;
    }

    void setBody(const std::string& content, const std::string& content_type = "text/html") {
        body = content;
        setHeader("Content-Type", content_type);
        setHeader("Content-Length", std::to_string(body.size()));
    }

    // Pour réponses 304 Not Modified
    void setNotModified(const std::string& etag) {
        setStatus(304);
        body.clear();
        setHeader("ETag", etag);
        setHeader("Content-Length", "0");
    }

    // Pour redirections
    void setRedirect(const std::string& location, int code = 302) {
        setStatus(code);
        body.clear();
        setHeader("Location", location);
        
        // Message HTML pour les navigateurs qui ne suivent pas automatiquement les redirections
        std::string redirect_body = "<html><head><title>Redirect</title></head><body>"
                                   "<h1>" + std::to_string(code) + " " + status_message + "</h1>"
                                   "<p>The document has moved <a href=\"" + location + "\">here</a>.</p>"
                                   "</body></html>";
        setBody(redirect_body);
    }

    // Pour réponses en chunks (gros fichiers)
    void setChunkedTransfer() {
        headers.erase("Content-Length");
        setHeader("Transfer-Encoding", "chunked");
    }

    // Support CORS (Cross-Origin Resource Sharing)
    void enableCORS(const std::string& origin = "*", 
                    const std::string& methods = "GET, POST, DELETE", 
                    const std::string& headers = "") {
        setHeader("Access-Control-Allow-Origin", origin);
        setHeader("Access-Control-Allow-Methods", methods);
        if (!headers.empty()) {
            setHeader("Access-Control-Allow-Headers", headers);
        }
    }

    std::string build() const {
        std::stringstream response;
        
        // Ligne de statut
        response << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";
        
        // Headers
        for (const auto& header : headers) {
            response << header.first << ": " << header.second << "\r\n";
        }
        
        // Séparation headers/body
        response << "\r\n";
        
        // Body (sauf pour les réponses HEAD qui n'en ont pas)
        if (!body.empty()) {
            response << body;
        }
        
        return response.str();
    }
    
    // Pour les méthodes HEAD (même réponse que GET mais sans body)
    std::string buildHeadResponse() const {
        std::stringstream response;
        
        // Ligne de statut
        response << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";
        
        // Headers
        for (const auto& header : headers) {
            response << header.first << ": " << header.second << "\r\n";
        }
        
        // Séparation headers/body sans inclure le body
        response << "\r\n";
        
        return response.str();
    }
};

Création de réponses d'erreur:

cppCopierHttpResponse createErrorResponse(int error_code, const std::string& message = "") {
    HttpResponse response;
    response.setStatus(error_code);
    
    std::string error_message = message.empty() ? response.status_message : message;
    std::string error_page = "<html><body><h1>" + 
                            std::to_string(error_code) + " " + 
                            error_message + 
                            "</h1></body></html>";
    
    response.setBody(error_page);
    return response;
}

Gestion des types MIME:

cppCopierstd::string getMimeType(const std::string& file_path) {
    static const std::map<std::string, std::string> mime_types = {
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".txt", "text/plain"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".pdf", "application/pdf"},
        {".zip", "application/zip"},
        {".gz", "application/gzip"},
        {".tar", "application/x-tar"},
        {".mp3", "audio/mpeg"},
        {".mp4", "video/mp4"},
        {".mpeg", "video/mpeg"},
        {".avi", "video/x-msvideo"},
        {".webm", "video/webm"},
        {".wav", "audio/wav"},
        {".ogg", "audio/ogg"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".ttf", "font/ttf"},
        {".otf", "font/otf"},
        {".eot", "application/vnd.ms-fontobject"},
        {".php", "text/html"} // PHP sera traité par CGI
    };

    size_t dot_pos = file_path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "application/octet-stream"; // Type par défaut
    }

    std::string extension = file_path.substr(dot_pos);
    auto it = mime_types.find(extension);
    return it != mime_types.end() ? it->second : "application/octet-stream";
}

Envoi de la réponse:

cppCopiervoid sendResponse(int client_fd, const HttpResponse& response, const HttpRequest& request) {
    std::string raw_response;
    
    // Pour les requêtes HEAD, n'envoyer que les headers
    if (request.method == "HEAD") {
        raw_response = response.buildHeadResponse();
    } else {
        raw_response = response.build();
    }
    
    size_t total_sent = 0;
    size_t to_send = raw_response.size();
    
    while (total_sent < to_send) {
        ssize_t sent = send(client_fd, raw_response.c_str() + total_sent, to_send - total_sent, 0);
        if (sent <= 0) {
            if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // Socket non prêt, réessayer après le prochain poll()
                break;
            } else {
                // Erreur réelle ou connexion fermée
                break;
            }
        }
        total_sent += sent;
    }
    
    // Stocker en mémoire les données non envoyées pour une reprise plus tard si nécessaire
    if (total_sent < to_send) {
        pending_responses[client_fd] = raw_response.substr(total_sent);
    }
}

Envoi de gros fichiers en mode chunked:

cppCopiervoid sendLargeFile(int client_fd, const std::string& file_path, const HttpRequest& request) {
    // Ouvrir le fichier
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        sendResponse(client_fd, createErrorResponse(404), request);
        return;
    }
    
    // Préparer l'en-tête de la réponse
    HttpResponse response;
    response.setStatus(200);
    response.setHeader("Content-Type", getMimeType(file_path));
    response.setChunkedTransfer();
    
    // Envoyer les headers
    std::string headers;
    if (request.method == "HEAD") {
        headers = response.buildHeadResponse();
        send(client_fd, headers.c_str(), headers.size(), 0);
        return; // Pour HEAD, on n'envoie pas le body
    } else {
        headers = "HTTP/1.1 200 OK\r\n";
        for (const auto& header : response.headers) {
            headers += header.first + ": " + header.second + "\r\n";
        }
        headers += "\r\n";
    }
    
    send(client_fd, headers.c_str(), headers.size(), 0);
    
    // Lire et envoyer le fichier par morceaux
    const size_t CHUNK_SIZE = 8192; // 8KB chunks
    char buffer[CHUNK_SIZE];
    char chunk_header[20]; // Pour stocker l'en-tête de chunk (taille en hex)
    
    while (file.good() && !file.eof()) {
        file.read(buffer, CHUNK_SIZE);
        std::streamsize bytes_read = file.gcount();
        
        if (bytes_read > 0) {
            // Formater l'en-tête du chunk (taille en hexadécimal)
            sprintf(chunk_header, "%zx\r\n", bytes_read);
            
            // Envoyer l'en-tête du chunk
            send(client_fd, chunk_header, strlen(chunk_header), 0);
            
            // Envoyer les données du chunk
            send(client_fd, buffer, bytes_read, 0);
            
            // Envoyer le terminateur de chunk
            send(client_fd, "\r\n", 2, 0);
        }
    }
    
    // Envoyer le chunk final (zéro)
    send(client_fd, "0\r\n\r\n", 5, 0);
}

Calcul d'ETag pour validation du cache:

cppCopierstd::string calculateETag(const std::string& file_path) {
    struct stat file_info;
    if (stat(file_path.c_str(), &file_info) != 0) {
        return "";
    }
    
    // Calculer un ETag basé sur la taille du fichier et la date de dernière modification
    std::stringstream etag;
    etag << "\"" << file_info.st_size << "-" << file_info.st_mtime << "\"";
    return etag.str();
}

Gestion des réponses 304 Not Modified:

cppCopierbool checkNotModified(const HttpRequest& request, const std::string& file_path, HttpResponse& response) {
    // Calculer l'ETag du fichier
    std::string etag = calculateETag(file_path);
    if (etag.empty()) {
        return false;
    }
    
    // Vérifier si le client a envoyé un If-None-Match
    auto it = request.headers.find("if-none-match");
    if (it != request.headers.end() && it->second == etag) {
        response.setNotModified(etag);
        return true;
    }
    
    // Ajouter l'ETag à la réponse pour les futures requêtes
    response.setHeader("ETag", etag);
    return false;
}
Points de validation

 Génère des réponses HTTP valides pour tous les codes de statut
 Gère correctement Content-Length et Content-Type
 Supporte les transferts en mode chunked pour les gros fichiers
 Gère correctement les réponses sans body (HEAD, 204, 304)
 Implémente la validation du cache avec ETag
 Gère les redirections (301, 302, 307, 308)
 Respecte le keep-alive ou ferme la connexion selon les headers
 Supporte CORS pour les API
 Gère les envois partiels avec reprise après poll()

Tests recommandés

Test réponse simple:

bashCopiercurl -v http://localhost:8080/

Test réponse d'erreur:

bashCopiercurl -v http://localhost:8080/nonexistent.html

Test méthode HEAD:

bashCopiercurl -v -I http://localhost:8080/

Test validation cache ETag:

bashCopier# Premier appel pour obtenir l'ETag
curl -v http://localhost:8080/index.html
# Deuxième appel avec l'ETag
curl -v -H "If-None-Match: \"12345-1234567890\"" http://localhost:8080/index.html

Test redirection:

bashCopiercurl -v -L http://localhost:8080/redirect

Test transfer chunked:

bashCopiercurl -v http://localhost:8080/large_file.mp4

Test CORS:

bashCopiercurl -v -H "Origin: http://example.com" -X OPTIONS http://localhost:8080/api/data
Bonnes pratiques

Toujours spécifier Content-Length sauf pour les transferts chunked
Utiliser le bon type MIME pour chaque fichier
Implémenter une gestion de cache efficace avec ETag
Fermer la connexion selon la valeur du header Connection
Gérer les erreurs de manière cohérente et informative
Supporter les requêtes HEAD correctement (mêmes headers que GET, sans body)
Considérer l'encodage GZIP pour les fichiers texte volumineux