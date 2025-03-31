#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include "http/parser/FormData.hpp"
#include <string>
#include <map>

/**
 * @brief Classe représentant une requête HTTP
 */
class HttpRequest {
public:
    // Constantes
    static const size_t MAX_BODY_SIZE = 10 * 1024 * 1024; // 10 Mo
    
    // Constructeur et destructeur
    HttpRequest();
    ~HttpRequest();

    // Méthodes principales
    void clear();
    bool parse(const std::string& raw_request);

    // Getters pour les données de base
    const std::string& getMethod() const { return method; }
    const std::string& getUri() const { return uri; }
    const std::string& getVersion() const { return version; }
    const std::string& getBody() const { return body; }
    const std::string& getQueryString() const { return query_string; }
    const std::map<std::string, std::string>& getHeaders() const { return headers; }
    
    // Getters pour les formulaires et fichiers (délégués à FormData)
    const std::map<std::string, std::string>& getFormValues() const;
    const std::map<std::string, UploadedFile>& getUploadedFiles() const;
    std::string getFormValue(const std::string& name) const;
    bool hasUploadedFile(const std::string& name) const;
    const UploadedFile* getUploadedFile(const std::string& name) const;

private:
    // Méthodes de parsing
    bool parseRequestLine(const std::string& line);
    bool parseHeaders(const std::string& headers_section);
    void parseQueryString();
    void parseFormBody();
    
    // Méthodes de validation
    bool validateRequest() const;
    
    // Données membres - Informations de base de la requête
    std::string method;      // GET, POST, DELETE
    std::string uri;         // /index.html
    std::string version;     // HTTP/1.1
    std::map<std::string, std::string> headers;
    std::string body;
    std::string query_string; // Paramètres après le ? dans l'URI
    
    // Données membres - Formulaires et fichiers
    FormData form_data;
};

#endif // HTTP_REQUEST_HPP 