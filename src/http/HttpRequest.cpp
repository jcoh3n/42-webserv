#include "http/HttpRequest.hpp"
#include "http/utils/HttpUtils.hpp"
#include "http/parser/FormParser.hpp"
#include <sstream>
#include <algorithm>

HttpRequest::HttpRequest() {}

HttpRequest::~HttpRequest() {}

void HttpRequest::clear() {
    method.clear();
    uri.clear();
    version.clear();
    headers.clear();
    body.clear();
    query_string.clear();
    form_data.clear();
}

bool HttpRequest::parse(const std::string& raw_request) {
    clear();

    // Trouver la fin de la ligne de requête
    size_t end_of_line = raw_request.find("\r\n");
    if (end_of_line == std::string::npos)
        return false;

    // Parser la première ligne
    if (!parseRequestLine(raw_request.substr(0, end_of_line)))
        return false;

    // Trouver la séparation entre headers et body
    size_t headers_end = raw_request.find("\r\n\r\n");
    if (headers_end == std::string::npos)
        return false;

    // Parser les headers
    if (!parseHeaders(raw_request.substr(end_of_line + 2, headers_end - end_of_line - 2)))
        return false;

    // Extraire le body s'il existe
    if (headers_end + 4 < raw_request.length())
        body = raw_request.substr(headers_end + 4);

    // Parser query string si présent dans l'URI
    parseQueryString();
    
    // Parser le body selon le Content-Type
    if (method == "POST" && !body.empty()) {
        parseFormBody();
    }

    return validateRequest();
}

bool HttpRequest::parseRequestLine(const std::string& line) {
    std::istringstream iss(line);
    if (!(iss >> method >> uri >> version))
        return false;

    // Convertir la méthode en majuscules
    std::transform(method.begin(), method.end(), method.begin(), ::toupper);
    return true;
}

bool HttpRequest::parseHeaders(const std::string& headers_section) {
    std::istringstream iss(headers_section);
    std::string line;

    while (std::getline(iss, line)) {
        // Ignorer les retours chariot Windows
        if (!line.empty() && line[line.length()-1] == '\r')
            line.erase(line.length()-1);
        
        // Ignorer les lignes vides
        if (line.empty())
            continue;

        // Trouver le séparateur ':'
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            return false;

        std::string name = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        // Nettoyer les espaces
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // Convertir le nom du header en minuscules
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);

        headers[name] = value;
    }
    return true;
}

void HttpRequest::parseQueryString() {
    size_t query_pos = uri.find('?');
    if (query_pos != std::string::npos) {
        query_string = uri.substr(query_pos + 1);
        uri = uri.substr(0, query_pos);
    }
}

void HttpRequest::parseFormBody() {
    std::map<std::string, std::string>::const_iterator it = headers.find("content-type");
    if (it == headers.end())
        return;
    
    const std::string& content_type = it->second;
    
    if (content_type.find("application/x-www-form-urlencoded") != std::string::npos) {
        FormParser::parseUrlEncoded(body, form_data);
    }
    else if (content_type.find("multipart/form-data") != std::string::npos) {
        // Extraire la boundary
        size_t boundary_pos = content_type.find("boundary=");
        if (boundary_pos != std::string::npos) {
            std::string boundary = content_type.substr(boundary_pos + 9);
            FormParser::parseMultipart(body, boundary, form_data);
        }
    }
}

bool HttpRequest::validateRequest() const {
    // Vérifier la méthode HTTP
    if (!HttpUtils::isMethodSupported(method))
        return false;
    
    // Vérifier l'URI
    if (!HttpUtils::isUriSafe(uri))
        return false;
    
    // Vérifier la version HTTP
    if (!HttpUtils::isVersionSupported(version))
        return false;
    
    // Vérifier la taille du contenu
    std::map<std::string, std::string>::const_iterator it = headers.find("content-length");
    if (it != headers.end()) {
        std::stringstream ss(it->second);
        size_t content_length = 0;
        ss >> content_length;
        if (!HttpUtils::isContentLengthValid(content_length, MAX_BODY_SIZE))
            return false;
    }
    
    return true;
}

// Délégation à FormData
const std::map<std::string, std::string>& HttpRequest::getFormValues() const {
    return form_data.getFormValues();
}

const std::map<std::string, UploadedFile>& HttpRequest::getUploadedFiles() const {
    return form_data.getUploadedFiles();
}

std::string HttpRequest::getFormValue(const std::string& name) const {
    return form_data.getFormValue(name);
}

bool HttpRequest::hasUploadedFile(const std::string& name) const {
    return form_data.hasUploadedFile(name);
}

const UploadedFile* HttpRequest::getUploadedFile(const std::string& name) const {
    return form_data.getUploadedFile(name);
} 