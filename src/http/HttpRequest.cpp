#include "http/HttpRequest.hpp"
#include "http/parser/FormParser.hpp"
#include "http/utils/HttpUtils.hpp"
#include "utils/Common.hpp"
#include <sstream>
#include <algorithm>

HttpRequest::HttpRequest() : max_body_size(DEFAULT_MAX_BODY_SIZE) {}

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
    
    // Trouver la fin de la première ligne
    size_t pos = raw_request.find("\r\n");
    if (pos == std::string::npos) {
        LOG_ERROR("Invalid request: no CRLF found");
        return false;
    }
    
    // Parser la première ligne (méthode, URI, version)
    if (!parseRequestLine(raw_request.substr(0, pos))) {
        return false;
    }
    
    // Parser les headers
    size_t body_start = raw_request.find("\r\n\r\n");
    if (body_start == std::string::npos) {
        LOG_ERROR("Invalid request: no empty line before body");
        return false;
    }
    
    // Extraire et parser les headers
    std::string headers_section = raw_request.substr(pos + 2, body_start - pos - 2);
    if (!parseHeaders(headers_section)) {
        return false;
    }
    
    // Extraire le body s'il existe
    if (body_start + 4 < raw_request.length()) {
        body = raw_request.substr(body_start + 4);
        
        // Vérifier la taille du body
        if (body.length() > max_body_size) {
            LOG_ERROR("Request body too large: " << body.length() << " bytes (max: " << max_body_size << ")");
            return false;
        }
    }
    
    // Parser l'URI pour extraire la query string
    parseQueryString();
    
    // Parser le body si c'est un formulaire
    if (!body.empty()) {
        parseFormBody();
    }
    
    return true;
}

bool HttpRequest::parseRequestLine(const std::string& line) {
    std::istringstream iss(line);
    
    // Extraire la méthode
    if (!(iss >> method)) {
        LOG_ERROR("Failed to parse HTTP method");
        return false;
    }
    
    // Extraire l'URI
    if (!(iss >> uri)) {
        LOG_ERROR("Failed to parse URI");
        return false;
    }
    
    // Extraire la version HTTP
    if (!(iss >> version)) {
        LOG_ERROR("Failed to parse HTTP version");
        return false;
    }
    
    // Vérifier la version HTTP
    if (version != "HTTP/1.1" && version != "HTTP/1.0") {
        LOG_ERROR("Unsupported HTTP version: " << version);
        return false;
    }
    
    return true;
}

bool HttpRequest::parseHeaders(const std::string& headers_section) {
    std::istringstream iss(headers_section);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Supprimer le \r à la fin de la ligne
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        // Ignorer les lignes vides
        if (line.empty()) {
            continue;
        }
        
        // Trouver le séparateur ': '
        size_t colon = line.find(": ");
        if (colon == std::string::npos) {
            LOG_ERROR("Invalid header format: " << line);
            return false;
        }
        
        // Extraire le nom et la valeur
        std::string name = line.substr(0, colon);
        std::string value = line.substr(colon + 2);
        
        // Convertir le nom en minuscules
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        
        // Ajouter le header
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
        std::string boundary = BoundaryExtractor::extract(content_type);
        if (!boundary.empty()) {
            FormParser::parseMultipart(body, boundary, form_data);
        }
        else {
            LOG_ERROR("Multipart form-data without boundary parameter");
        }
    }
}

std::string HttpRequest::extractBoundary(const std::string& content_type) const {
    if (content_type.find("multipart/form-data") == std::string::npos) {
        return "";
    }
    
    size_t pos = content_type.find("boundary=");
    if (pos == std::string::npos) {
        return "";
    }
    
    std::string boundary = content_type.substr(pos + 9); // "boundary=" a 9 caractères
    
    // Supprimer les guillemets si présents
    if (boundary.size() > 0 && boundary[0] == '"') {
        boundary = boundary.substr(1, boundary.size() - 2);
    }
    
    return boundary;
}