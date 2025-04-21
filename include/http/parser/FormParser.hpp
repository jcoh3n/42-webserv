#ifndef FORM_PARSER_HPP
#define FORM_PARSER_HPP

#include "http/parser/FormData.hpp"
#include <string>
#include <map>

// Classe utilitaire pour gérer les boundaries
class BoundaryExtractor {
public:
    static std::string extract(const std::string& content_type);
    static std::string clean(const std::string& boundary);
private:
    static void removeQuotes(std::string& boundary);
    static void removeSemicolonAndAfter(std::string& boundary);
    static void trimWhitespace(std::string& boundary);
};

// Classe utilitaire pour gérer les headers des parties multipart
class MultipartHeaderParser {
public:
    struct HeaderInfo {
        std::string name;
        std::string filename;
        std::string content_type;
    };
    
    static HeaderInfo parse(const std::string& headers);
private:
    static void parseContentDisposition(const std::string& header_value, HeaderInfo& info);
    static void parseContentType(const std::string& header_value, HeaderInfo& info);
};

/**
 * @brief Classe pour parser différents types de formulaires
 */
class FormParser {
public:
    /**
     * @brief Parse des données URL-encoded
     *
     * @param body Corps de la requête
     * @param form_data Objet FormData à remplir
     */
    static void parseUrlEncoded(const std::string& body, FormData& form_data);
    
    /**
     * @brief Parse des données multipart/form-data
     *
     * @param body Corps de la requête
     * @param boundary Boundary séparant les parties
     * @param form_data Objet FormData à remplir
     */
    static void parseMultipart(const std::string& body, const std::string& boundary, FormData& form_data);

private:
    /**
     * @brief Parse une partie multipart
     *
     * @param part Partie à parser
     * @param form_data Objet FormData à remplir
     */
    static void parseMultipartPart(const std::string& part, FormData& form_data);
    
    // Constructeur privé pour empêcher l'instanciation
    FormParser() {}
};

#endif // FORM_PARSER_HPP 