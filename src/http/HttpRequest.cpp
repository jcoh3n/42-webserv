#include "http/HttpRequest.hpp"
#include "http/utils/HttpUtils.hpp"
#include "http/parser/FormParser.hpp"
#include <sstream>
#include <algorithm>

/**
 * @brief Constructeur par défaut
 */
HttpRequest::HttpRequest() {}

/**
 * @brief Destructeur
 */
HttpRequest::~HttpRequest() {}

/**
 * @brief Réinitialise toutes les propriétés de la requête
 * 
 * Vide toutes les variables membres pour permettre la réutilisation de l'objet.
 */
void HttpRequest::clear() {
    method.clear();
    uri.clear();
    version.clear();
    headers.clear();
    body.clear();
    query_string.clear();
    form_data.clear();
}

/**
 * @brief Parse une requête HTTP à partir d'une chaîne brute
 * @param raw_request La chaîne contenant la requête HTTP complète
 * @return true si le parsing a réussi, false sinon
 * 
 * Procède en plusieurs étapes :
 * 1. Parse la ligne de requête (méthode, URI, version)
 * 2. Parse les en-têtes
 * 3. Extrait le body
 * 4. Parse les paramètres d'URL et le body selon le Content-Type
 */
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

/**
 * @brief Parse la première ligne de la requête HTTP
 * @param line La ligne contenant la méthode, l'URI et la version
 * @return true si le parsing a réussi, false sinon
 * 
 * Extrait la méthode HTTP, l'URI et la version du protocole.
 * Convertit également la méthode en majuscules.
 */
bool HttpRequest::parseRequestLine(const std::string& line) {
    std::istringstream iss(line);
    if (!(iss >> method >> uri >> version))
        return false;

    // Convertir la méthode en majuscules
    std::transform(method.begin(), method.end(), method.begin(), ::toupper);
    return true;
}

/**
 * @brief Parse les en-têtes HTTP
 * @param headers_section La section de la requête contenant les en-têtes
 * @return true si le parsing a réussi, false sinon
 * 
 * Extrait toutes les paires clé-valeur des en-têtes HTTP
 * et les stocke dans la map des headers.
 */
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

/**
 * @brief Extrait et parse les paramètres d'URL
 * 
 * Sépare l'URI de la query string (tout ce qui suit '?')
 * et stocke la query string séparément.
 */
void HttpRequest::parseQueryString() {
    size_t query_pos = uri.find('?');
    if (query_pos != std::string::npos) {
        query_string = uri.substr(query_pos + 1);
        uri = uri.substr(0, query_pos);
    }
}

/**
 * @brief Parse le body de la requête selon son Content-Type
 * 
 * Traite le body différemment selon qu'il s'agit d'un formulaire
 * application/x-www-form-urlencoded ou multipart/form-data.
 */
void HttpRequest::parseFormBody() {
    std::map<std::string, std::string>::const_iterator it = headers.find("content-type");
    if (it == headers.end())
        return;
    
    const std::string& content_type = it->second;
    
    if (content_type.find("application/x-www-form-urlencoded") != std::string::npos) {
        FormParser::parseUrlEncoded(body, form_data);
    }
    else if (content_type.find("multipart/form-data") != std::string::npos) {
        // Multipart/form-data parsing and file upload feature has been temporarily disabled
        // This will be reimplemented in a future version of the server
        // No parsing is performed for this content type
    }
}

/**
 * @brief Valide la requête HTTP
 * @return true si la requête est valide, false sinon
 * 
 * Vérifie que la méthode, l'URI et la version HTTP sont valides.
 * Vérifie également que la taille du contenu est acceptable.
 */
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