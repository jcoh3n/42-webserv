#include "http/utils/HttpUtils.hpp"
#include <algorithm>

// Définition des constantes statiques
const std::string HttpUtils::ALLOWED_METHODS[] = {"GET", "POST", "DELETE"};
const size_t HttpUtils::METHOD_COUNT = sizeof(ALLOWED_METHODS) / sizeof(ALLOWED_METHODS[0]);

std::string HttpUtils::urlDecode(const std::string& encoded) {
    std::string result;
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            int value;
            std::istringstream is(encoded.substr(i + 1, 2));
            if (is >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += encoded[i];
            }
        } else if (encoded[i] == '+') {
            result += ' ';
        } else {
            result += encoded[i];
        }
    }
    return result;
}

bool HttpUtils::isUriSafe(const std::string& uri) {
    // Vérification basique : pas de '..' pour éviter la traversée de répertoire
    return !uri.empty() && uri[0] == '/' && uri.find("..") == std::string::npos;
}

bool HttpUtils::isMethodSupported(const std::string& method) {
    // Accepter toutes les méthodes HTTP valides
    // Une méthode HTTP valide ne doit contenir que des caractères alphanumériques en majuscules
    for (size_t i = 0; i < method.length(); ++i) {
        if (!isupper(method[i]) && !isdigit(method[i]))
            return false;
    }
    return !method.empty();
}

bool HttpUtils::isVersionSupported(const std::string& version) {
    return version == "HTTP/1.1" || version == "HTTP/1.0";
}

bool HttpUtils::isContentLengthValid(size_t content_length, size_t max_size) {
    return content_length <= max_size;
} 