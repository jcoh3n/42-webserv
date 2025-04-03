#include "config/ConfigParser.hpp"
#include "utils/Common.hpp"
#include <cctype>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib> // for atoi, atol
#include <cstring> // for strcmp
#include <algorithm>

namespace HTTP {

/**
 * @brief Parse un fichier de configuration et retourne la configuration résultante
 * @param filename Chemin vers le fichier de configuration
 * @return La configuration complète du webserv
 * @throw std::runtime_error Si une erreur de parsing survient
 */
WebservConfig ConfigParser::parseFile(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + filename);
    }

    WebservConfig config;
    std::string line;
    int line_number = 0;

    ServerConfig current_server;
    LocationConfig current_location;
    std::string section = "";
    std::string location_path = "";
    bool has_server = false;
    bool is_parsing_server = false;
    size_t server_index = 0;

    try {
        while (std::getline(file, line)) {
            line_number++;
            
            // Ignorer les lignes vides et les commentaires
            line = trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // Nouvelle section
            if (line[0] == '[' && line[line.length() - 1] == ']') {
                std::string new_section = line.substr(1, line.length() - 2);
                
                // Finir la section précédente
                if (section == "server") {
                    config.servers.push_back(current_server);
                    server_index = config.servers.size() - 1;
                    has_server = true;
                    is_parsing_server = true;
                } else if (section == "location" && is_parsing_server) {
                    // Si nous étions en train de traiter une location à l'intérieur d'un serveur
                    if (server_index < config.servers.size()) {
                        config.servers[server_index].locations[location_path] = current_location;
                    } else {
                        throw std::runtime_error("Internal error: Invalid server index");
                    }
                }
                
                // Commencer une nouvelle section
                if (new_section.substr(0, 9) == "location ") {
                    if (!is_parsing_server) {
                        throw std::runtime_error("Location must be inside a server section");
                    }
                    section = "location";
                    location_path = new_section.substr(9);
                    current_location = LocationConfig();
                } else if (new_section == "server") {
                    section = "server";
                    current_server = ServerConfig();
                    is_parsing_server = false; // Réinitialisé à true après l'ajout du serveur
                } else {
                    throw std::runtime_error("Unknown section: " + new_section);
                }
            } else {
                // Directive clé=valeur
                if (section == "server") {
                    parseLine(line, current_server, current_location, section, location_path);
                } else if (section == "location" && is_parsing_server) {
                    parseLine(line, current_server, current_location, section, location_path);
                } else {
                    throw std::runtime_error("Directive outside of valid section: " + line);
                }
            }
        }
        
        // Finir la dernière section
        if (section == "server") {
            config.servers.push_back(current_server);
            has_server = true;
        } else if (section == "location" && is_parsing_server) {
            if (server_index < config.servers.size()) {
                config.servers[server_index].locations[location_path] = current_location;
            } else {
                throw std::runtime_error("Internal error: Invalid server index");
            }
            
            // Ne pas ajouter plusieurs fois le même serveur
            if (!has_server) {
                config.servers.push_back(current_server);
                has_server = true;
            }
        }
    } catch (const std::exception& e) {
        std::ostringstream errorMsg;
        errorMsg << "Error on line " << line_number << ": " << e.what();
        throw std::runtime_error(errorMsg.str());
    }
    
    if (!has_server) {
        throw std::runtime_error("No server section defined in config file");
    }

    validateConfig(config);
    return config;
}

void ConfigParser::parseLine(const std::string& line, ServerConfig& current_server, 
                           LocationConfig& current_location, std::string& section,
                           std::string& location_path) {
    // Utiliser location_path pour éviter le warning (non utilisé directement dans cette fonction)
    (void)location_path;
    
    // Format: key=value
    size_t pos = line.find('=');
    if (pos == std::string::npos) {
        throw std::runtime_error("Invalid directive (format should be key=value): " + line);
    }
    
    std::string key = trim(line.substr(0, pos));
    std::string value = trim(line.substr(pos + 1));
    
    if (section.empty()) {
        throw std::runtime_error("Directive outside of any section: " + line);
    } else if (section == "server") {
        // Directives de serveur
        if (key == "port") {
            current_server.port = atoi(value.c_str());
        } else if (key == "host") {
            current_server.host = value;
        } else if (key == "server_name") {
            current_server.server_names = split(value, ' ');
        } else if (key == "root") {
            current_server.root_directory = value;
        } else if (key == "index") {
            current_server.index_files = split(value, ' ');
        } else if (key == "client_max_body_size") {
            current_server.client_max_body_size = parseSize(value);
        } else if (key == "error_page") {
            // Format: code file_path
            std::vector<std::string> parts = split(value, ' ');
            if (parts.size() < 2) {
                throw std::runtime_error("Invalid error_page format (should be: error_page=code file_path)");
            }
            int code = atoi(parts[0].c_str());
            current_server.error_pages[code] = parts[1];
        } else {
            throw std::runtime_error("Unknown server directive: " + key);
        }
    } else if (section == "location") {
        // Directives de location
        if (key == "allowed_methods") {
            current_location.allowed_methods = split(value, ' ');
        } else if (key == "root") {
            current_location.root_override = value;
        } else if (key == "index") {
            current_location.index_files = split(value, ' ');
        } else if (key == "autoindex") {
            current_location.autoindex = (value == "on" || value == "true" || value == "1");
        } else if (key == "upload_directory") {
            current_location.upload_directory = value;
        } else if (key == "return") {
            // Format: code url
            std::vector<std::string> parts = split(value, ' ');
            if (parts.size() < 2) {
                throw std::runtime_error("Invalid return format (should be: return=code url)");
            }
            current_location.redirect_code = atoi(parts[0].c_str());
            current_location.redirect_url = parts[1];
        } else if (key == "cgi") {
            current_location.cgi_extensions = split(value, ' ');
        } else if (key == "alias") {
            current_location.alias = value;
        } else {
            throw std::runtime_error("Unknown location directive: " + key);
        }
    }
}

std::vector<std::string> ConfigParser::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

std::string ConfigParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, last - first + 1);
}

size_t ConfigParser::parseSize(const std::string& size_str) {
    if (size_str.empty()) {
        return 0;
    }
    
    size_t len = size_str.length();
    
    if (size_str.find_first_not_of("0123456789") == std::string::npos) {
        return static_cast<size_t>(atol(size_str.c_str()));
    }
    
    size_t value = static_cast<size_t>(atol(size_str.substr(0, len - 1).c_str()));
    char unit = size_str[len - 1];
    
    switch (unit) {
        case 'k':
        case 'K':
            return value * 1024;
        case 'm':
        case 'M':
            return value * 1024 * 1024;
        case 'g':
        case 'G':
            return value * 1024 * 1024 * 1024;
        default:
            throw std::runtime_error("Invalid size unit");
    }
}

bool ConfigParser::fileExists(const std::string& path) {
    std::ifstream file(path.c_str());
    return file.good();
}

bool ConfigParser::directoryExists(const std::string& path) {
    std::ifstream dir((path + "/.").c_str());
    return dir.good();
}

void ConfigParser::validateConfig(const WebservConfig& config) {
    // Vérifier qu'il y a au moins un serveur
    if (config.servers.empty()) {
        throw std::runtime_error("No servers defined in config");
    }

    // Vérifier les configurations en double (même host:port)
    std::map<std::pair<std::string, int>, int> port_counts;
    
    for (size_t i = 0; i < config.servers.size(); ++i) {
        const ServerConfig& server = config.servers[i];
        std::pair<std::string, int> server_addr(server.host, server.port);
        
        if (port_counts.find(server_addr) == port_counts.end()) {
            port_counts[server_addr] = 1;
        } else {
            port_counts[server_addr]++;
        }
    }
    
    // Vérifier les serveurs avec des server_names en double
    std::map<std::pair<std::string, int>, int>::iterator port_it;
    for (port_it = port_counts.begin(); port_it != port_counts.end(); ++port_it) {
        if (port_it->second > 1) {
            // Plusieurs serveurs sur le même host:port
            std::vector<const ServerConfig*> servers_on_port;
            
            for (size_t i = 0; i < config.servers.size(); ++i) {
                const ServerConfig& server = config.servers[i];
                if (server.host == port_it->first.first && server.port == port_it->first.second) {
                    servers_on_port.push_back(&server);
                }
            }
            
            // Vérifier les server_names en double
            std::map<std::string, int> name_counts;
            
            for (size_t i = 0; i < servers_on_port.size(); ++i) {
                const ServerConfig* server = servers_on_port[i];
                if (server->server_names.empty()) {
                    LOG_WARNING("Server with no server_name on " << port_it->first.first << ":" 
                              << port_it->first.second);
                }
                
                for (size_t j = 0; j < server->server_names.size(); ++j) {
                    std::string name = server->server_names[j];
                    if (name_counts.find(name) == name_counts.end()) {
                        name_counts[name] = 1;
                    } else {
                        name_counts[name]++;
                    }
                }
            }
            
            // Vérifier les server_names en double
            std::map<std::string, int>::iterator name_it;
            for (name_it = name_counts.begin(); name_it != name_counts.end(); ++name_it) {
                if (name_it->second > 1) {
                    std::ostringstream error_msg;
                    error_msg << "Duplicate server_name '" << name_it->first
                              << "' for " << port_it->first.first << ":" 
                              << port_it->first.second;
                    throw std::runtime_error(error_msg.str());
                }
            }
        }
    }
    
    // Vérifier chaque configuration de serveur
    for (size_t i = 0; i < config.servers.size(); ++i) {
        const ServerConfig& server = config.servers[i];
        
        // Vérifier les chemins de locations en double
        std::map<std::string, int> location_counts;
        
        std::map<std::string, LocationConfig>::const_iterator loc_it;
        for (loc_it = server.locations.begin(); loc_it != server.locations.end(); ++loc_it) {
            if (location_counts.find(loc_it->first) == location_counts.end()) {
                location_counts[loc_it->first] = 1;
            } else {
                location_counts[loc_it->first]++;
            }
            
            if (location_counts[loc_it->first] > 1) {
                std::ostringstream error_msg;
                error_msg << "Duplicate location '" << loc_it->first
                          << "' in server " << server.host << ":" 
                          << server.port;
                throw std::runtime_error(error_msg.str());
            }
            
            // Vérifier que les répertoires existent
            const LocationConfig& location = loc_it->second;
            if (!location.root_override.empty()) {
                if (!directoryExists(location.root_override)) {
                    LOG_WARNING("Root directory doesn't exist: " << location.root_override);
                }
            }
            
            if (!location.upload_directory.empty()) {
                if (!directoryExists(location.upload_directory)) {
                    LOG_WARNING("Upload directory doesn't exist: " << location.upload_directory);
                }
            }
        }
        
        // Vérifier le répertoire racine du serveur
        if (!directoryExists(server.root_directory)) {
            LOG_WARNING("Root directory doesn't exist: " << server.root_directory);
        }
        
        // Vérifier les pages d'erreur
        std::map<int, std::string>::const_iterator err_it;
        for (err_it = server.error_pages.begin(); err_it != server.error_pages.end(); ++err_it) {
            std::string page_path = server.root_directory + "/" + err_it->second;
            if (!fileExists(page_path)) {
                LOG_WARNING("Error page doesn't exist: " << page_path);
            }
        }
    }
}

const ServerConfig& selectVirtualServer(const WebservConfig& config, const std::string& hostname, int port) {
    std::vector<const ServerConfig*> matching_servers;
    
    for (size_t i = 0; i < config.servers.size(); ++i) {
        const ServerConfig& server = config.servers[i];
        if (server.port == port) {
            // Correspondance exacte d'hôte
            if (server.host == hostname) {
                matching_servers.push_back(&server);
            }
            // Serveur écoutant sur toutes les interfaces
            else if (server.host == "0.0.0.0") {
                matching_servers.push_back(&server);
            }
        }
    }
    
    if (matching_servers.empty()) {
        std::ostringstream error_msg;
        error_msg << "No server configured for " << hostname << ":" << port;
        throw std::runtime_error(error_msg.str());
    }
    
    // S'il n'y a qu'un seul serveur, le retourner
    if (matching_servers.size() == 1) {
        return *matching_servers[0];
    }
    
    // Plusieurs serveurs correspondent, nous devons sélectionner celui avec le bon server_name
    // Chercher d'abord une correspondance exacte de server_name
    for (size_t i = 0; i < matching_servers.size(); ++i) {
        const ServerConfig* server = matching_servers[i];
        for (size_t j = 0; j < server->server_names.size(); ++j) {
            const std::string& server_name = server->server_names[j];
            if (server_name == hostname) {
                return *server;
            }
        }
    }
    
    // Pas de correspondance exacte, retourner le premier serveur (par défaut)
    return *matching_servers[0];
}

const LocationConfig& selectLocation(const ServerConfig& server, const std::string& uri) {
    const LocationConfig* best_location = NULL;
    size_t best_match_len = 0;
    
    std::map<std::string, LocationConfig>::const_iterator it;
    for (it = server.locations.begin(); it != server.locations.end(); ++it) {
        const std::string& location_path = it->first;
        
        // Correspondance exacte
        if (location_path == uri) {
            return it->second;
        }
        
        // Vérifier une correspondance de préfixe
        if (uri.substr(0, location_path.length()) == location_path && 
            (uri.length() == location_path.length() || uri[location_path.length()] == '/' || 
            location_path[location_path.length() - 1] == '/')) {
            if (location_path.length() > best_match_len) {
                best_match_len = location_path.length();
                best_location = &it->second;
            }
        }
    }
    
    // Si nous avons trouvé une location correspondante, la retourner
    if (best_location != NULL) {
        return *best_location;
    }
    
    // Vérifier s'il y a une location racine (/)
    std::map<std::string, LocationConfig>::const_iterator root_location = server.locations.find("/");
    if (root_location != server.locations.end()) {
        return root_location->second;
    }
    
    // Aucune location correspondante, lever une erreur
    throw std::runtime_error("No matching location for URI: " + uri);
}

}  // namespace HTTP 