#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "config/ConfigTypes.hpp"
#include "utils/Common.hpp"
#include <string>
#include <vector>

namespace HTTP {

/**
 * @brief Classe pour parser les fichiers de configuration simplifiés
 * Format: clé=valeur, un par ligne
 * Les sections sont délimitées par [section]
 */
class ConfigParser {
public:
    /**
     * @brief Parse un fichier de configuration et retourne la configuration résultante
     * @param filename Chemin vers le fichier de configuration
     * @return La configuration complète du webserv
     * @throw std::runtime_error Si une erreur de parsing survient
     */
    WebservConfig parseFile(const std::string& filename);

private:
    /**
     * @brief Parse une ligne de configuration
     * @param line La ligne à parser
     * @param current_server Le serveur en cours de configuration
     * @param current_location La location en cours de configuration
     * @param section La section en cours (server, location, etc.)
     * @param location_path Le chemin de la location en cours
     */
    void parseLine(const std::string& line, ServerConfig& current_server, 
                   LocationConfig& current_location, std::string& section, 
                   std::string& location_path);
                   
    /**
     * @brief Valide la configuration complète
     * @param config La configuration à valider
     * @throw std::runtime_error Si la configuration est invalide
     */
    void validateConfig(const WebservConfig& config);
    
    /**
     * @brief Analyse une taille en octets à partir d'une chaîne
     * @param size_str La chaîne représentant la taille (ex: "10M", "1G")
     * @return La taille en octets
     */
    size_t parseSize(const std::string& size_str);
    
    /**
     * @brief Vérifie si un répertoire existe
     * @param path Le chemin du répertoire
     * @return true si le répertoire existe, false sinon
     */
    bool directoryExists(const std::string& path);
    
    /**
     * @brief Vérifie si un fichier existe
     * @param path Le chemin du fichier
     * @return true si le fichier existe, false sinon
     */
    bool fileExists(const std::string& path);
    
    /**
     * @brief Supprime les espaces au début et à la fin d'une chaîne
     * @param str La chaîne à traiter
     * @return La chaîne sans espaces au début et à la fin
     */
    std::string trim(const std::string& str);
    
    /**
     * @brief Divise une chaîne en fonction d'un délimiteur
     * @param str La chaîne à diviser
     * @param delimiter Le délimiteur
     * @return Un vecteur contenant les parties de la chaîne
     */
    std::vector<std::string> split(const std::string& str, char delimiter);
};

/**
 * @brief Sélectionne le serveur virtuel approprié en fonction de l'hôte et du port
 * @param config La configuration complète
 * @param hostname L'hôte demandé
 * @param port Le port demandé
 * @return La configuration du serveur sélectionné
 * @throw std::runtime_error Si aucun serveur ne correspond
 */
const ServerConfig& selectVirtualServer(const WebservConfig& config, 
                                       const std::string& hostname, 
                                       int port);

/**
 * @brief Sélectionne la location appropriée en fonction de l'URI
 * @param server La configuration du serveur
 * @param uri L'URI demandée
 * @return La configuration de la location sélectionnée
 */
const LocationConfig& selectLocation(const ServerConfig& server, const std::string& uri);

} // namespace HTTP

#endif // CONFIG_PARSER_HPP 