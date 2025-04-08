#ifndef FORM_PARSER_HPP
#define FORM_PARSER_HPP

#include "http/parser/FormData.hpp"
#include <string>
#include <map>

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
     * @brief Parse des données multipart/form-data (TEMPORARILY DISABLED)
     *
     * @param body Corps de la requête
     * @param boundary Boundary séparant les parties
     * @param form_data Objet FormData à remplir
     */
    static void parseMultipart(const std::string& body, const std::string& boundary, FormData& form_data);

private:
    /**
     * @brief Parse une partie multipart (TEMPORARILY DISABLED)
     *
     * @param part Partie à parser
     * @param form_data Objet FormData à remplir
     */
    static void parseMultipartPart(const std::string& part, FormData& form_data);
    
    /**
     * @brief Extrait les headers d'une partie multipart (TEMPORARILY DISABLED)
     *
     * @param part_headers Headers de la partie
     * @param name Nom du champ (out)
     * @param filename Nom du fichier (out)
     * @param content_type Type de contenu (out)
     */
    static void extractPartHeaders(const std::string& part_headers, 
                           std::string& name, 
                           std::string& filename, 
                           std::string& content_type);
    
    // Constructeur privé pour empêcher l'instanciation
    FormParser() {}
};

#endif // FORM_PARSER_HPP 