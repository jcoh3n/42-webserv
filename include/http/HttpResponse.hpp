#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <string>
#include <map>
#include <sstream>

// Forward declaration
class HttpRequest;

/**
 * @brief Classe représentant une réponse HTTP
 */
class HttpResponse {
public:
    // Constructeur et destructeur
    HttpResponse();
    ~HttpResponse();

    // Méthodes pour définir l'état de la réponse
    void setStatus(int code, const std::string& message = "");
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& content, const std::string& content_type = "text/html");

    // Méthodes pour les cas spéciaux de réponses
    void setNotModified(const std::string& etag);
    void setRedirect(const std::string& location, int code = 302);
    void setChunkedTransfer();
    void enableCORS(const std::string& origin = "*", 
                    const std::string& methods = "GET, POST, DELETE", 
                    const std::string& headers = "");

    // Méthodes pour construire la réponse finale
    std::string build() const;
    std::string buildHeadResponse() const;

    // Getters
    int getStatusCode() const { return status_code; }
    const std::string& getStatusMessage() const { return status_message; }
    const std::map<std::string, std::string>& getHeaders() const { return headers; }
    const std::string& getBody() const { return body; }

    // Méthodes statiques pour créer des réponses spécifiques
    static HttpResponse createError(int error_code, const std::string& message = "");

private:
    int status_code;
    std::string status_message;
    std::map<std::string, std::string> headers;
    std::string body;
};

// Fonctions utilitaires pour les réponses HTTP
std::string getMimeType(const std::string& file_path);
std::string calculateETag(const std::string& file_path);
bool checkNotModified(const HttpRequest& request, const std::string& file_path, HttpResponse& response);

#endif // HTTP_RESPONSE_HPP 