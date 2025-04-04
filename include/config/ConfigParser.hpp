#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "config/ConfigTypes.hpp"
#include "utils/Common.hpp"
#include <string>
#include <vector>
#include <map>

/**
 * @brief Classe pour parser les fichiers de configuration de style Nginx
 * Format supporté: blocs avec accolades, directives clé=valeur ou clé valeur
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
     * @brief Parse une ligne de configuration avec la nouvelle syntaxe à accolades
     * @param line La ligne à parser
     * @param config La configuration en cours de construction
     * @param current_server Le serveur en cours de configuration
     * @param current_location La location en cours de configuration
     * @param location_path Le chemin de la location en cours
     * @param in_server Indicateur si on est dans un bloc server
     * @param in_location Indicateur si on est dans un bloc location
     * @param brace_level Niveau d'imbrication des accolades
     */
    void parseNewSyntax(const std::string& line, WebservConfig& config,
                     ServerConfig& current_server, LocationConfig& current_location,
                     std::string& location_path, bool& in_server, bool& in_location,
                     int& brace_level);

    /**
     * @brief Parse une directive de type clé=valeur ou clé valeur
     * @param line La ligne à parser
     * @param in_server Indicateur si on est dans un bloc server
     * @param in_location Indicateur si on est dans un bloc location
     * @param server Le serveur en cours de configuration
     * @param location La location en cours de configuration
     */
    void parseDirective(const std::string& line, bool in_server, bool in_location,
                     ServerConfig& server, LocationConfig& location);

    /**
     * @brief Traite une directive de serveur
     * @param key Clé de la directive
     * @param value Valeur de la directive
     * @param server Configuration du serveur à modifier
     */
    void processServerDirective(const std::string& key, const std::string& value,
                             ServerConfig& server);

    /**
     * @brief Traite une directive de location
     * @param key Clé de la directive
     * @param value Valeur de la directive
     * @param location Configuration de la location à modifier
     */
    void processLocationDirective(const std::string& key, const std::string& value,
                               LocationConfig& location);

    // Méthodes utilitaires
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::string trim(const std::string& str);
    size_t parseSize(const std::string& size_str);
    bool fileExists(const std::string& path);
    bool directoryExists(const std::string& path);

    // Méthodes de validation
    void validateConfig(const WebservConfig& config);
    void countServersByAddress(const WebservConfig& config, std::map<std::pair<std::string, int>, int>& port_counts);
    void checkDuplicateServerNames(const WebservConfig& config, const std::map<std::pair<std::string, int>, int>& port_counts);
    void validateServerConfigs(const WebservConfig& config);
    void checkDuplicateLocations(const ServerConfig& server);
    void validateLocationDirectories(const LocationConfig& location);
    void checkErrorPages(const ServerConfig& server);
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
 * @brief Recherche les serveurs correspondant à un port donné
 * @param config La configuration complète
 * @param port Le port recherché
 * @param matching_servers Liste des serveurs correspondants (sortie)
 */
void findServersByPort(const WebservConfig& config, int port,
                     std::vector<const ServerConfig*>& matching_servers);

/**
 * @brief Recherche un serveur par nom dans une liste de serveurs
 * @param servers La liste des serveurs à vérifier
 * @param hostname Le hostname recherché
 * @return Le serveur correspondant ou NULL si aucun ne correspond
 */
const ServerConfig* findServerByName(const std::vector<const ServerConfig*>& servers, 
                                   const std::string& hostname);

/**
 * @brief Sélectionne la location appropriée en fonction de l'URI
 * @param server La configuration du serveur
 * @param uri L'URI demandée
 * @return La configuration de la location sélectionnée
 */
const LocationConfig& selectLocation(const ServerConfig& server, const std::string& uri);

/**
 * @brief Trouve la meilleure correspondance de location pour une URI
 * @param server La configuration du serveur
 * @param uri L'URI demandée
 * @return Pointeur vers la location la plus appropriée ou NULL si aucune ne correspond
 */
const LocationConfig* findBestLocationMatch(const ServerConfig& server, const std::string& uri);

/**
 * @brief Vérifie si un chemin de location correspond à un préfixe d'URI
 * @param uri L'URI à vérifier
 * @param location_path Le chemin de location à comparer
 * @return true si le chemin correspond, false sinon
 */
bool isLocationPrefixMatch(const std::string& uri, const std::string& location_path);

#endif // CONFIG_PARSER_HPP 