#include "config/ConfigParser.hpp"
#include "config/ConfigTypes.hpp"
#include "utils/Common.hpp"
#include <sstream>

void ConfigParser::validateConfig(const WebservConfig& config) {
    // Vérifier qu'il y a au moins un serveur
    if (config.servers.empty()) {
        throw std::runtime_error("No servers defined in config");
    }

    // Vérifier les configurations en double (même host:port)
    std::map<std::pair<std::string, int>, int> port_counts;
    countServersByAddress(config, port_counts);
    
    // Vérifier les serveurs avec des server_names en double
    checkDuplicateServerNames(config, port_counts);
    
    // Vérifier chaque configuration de serveur
    validateServerConfigs(config);
}

// Compte le nombre de serveurs par couple host:port
void ConfigParser::countServersByAddress(const WebservConfig& config, 
                                      std::map<std::pair<std::string, int>, int>& port_counts) {
    for (size_t i = 0; i < config.servers.size(); ++i) {
        const ServerConfig& server = config.servers[i];
        std::pair<std::string, int> server_addr(server.host, server.port);
        
        if (port_counts.find(server_addr) == port_counts.end()) {
            port_counts[server_addr] = 1;
        } else {
            port_counts[server_addr]++;
        }
    }
}

// Vérifie les noms de serveurs en double sur les mêmes host:port
void ConfigParser::checkDuplicateServerNames(const WebservConfig& config, 
                                          const std::map<std::pair<std::string, int>, int>& port_counts) {
    std::map<std::pair<std::string, int>, int>::const_iterator port_it;
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
}

// Valide les configurations individuelles des serveurs
void ConfigParser::validateServerConfigs(const WebservConfig& config) {
    for (size_t i = 0; i < config.servers.size(); ++i) {
        const ServerConfig& server = config.servers[i];
        
        // Vérifier les chemins de locations en double
        checkDuplicateLocations(server);
        
        // Vérifier le répertoire racine du serveur
        if (!directoryExists(server.root_directory)) {
            LOG_WARNING("Root directory doesn't exist: " << server.root_directory);
        }
        
        // Vérifier les pages d'erreur
        checkErrorPages(server);
    }
}

// Vérifie les chemins de location en double et la validité des répertoires
void ConfigParser::checkDuplicateLocations(const ServerConfig& server) {
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
        validateLocationDirectories(loc_it->second);
    }
}

// Vérifie que les répertoires d'une location existent
void ConfigParser::validateLocationDirectories(const LocationConfig& location) {
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

// Vérifie que les pages d'erreur existent
void ConfigParser::checkErrorPages(const ServerConfig& server) {
    std::map<int, std::string>::const_iterator err_it;
    for (err_it = server.error_pages.begin(); err_it != server.error_pages.end(); ++err_it) {
        std::string page_path = server.root_directory + "/" + err_it->second;
        if (!fileExists(page_path)) {
            LOG_WARNING("Error page doesn't exist: " << page_path);
        }
    }
} 