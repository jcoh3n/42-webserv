#include "config/ConfigParser.hpp"
#include "config/ConfigTypes.hpp"
#include "utils/Common.hpp"
#include <sstream>

void ConfigParser::validateConfig(const WebservConfig& config) {
    // Vérification de base
    if (config.servers.empty()) {
        throw std::runtime_error("No servers defined in configuration");
    }

    // Vérifier les configurations en double (même host:port)
    std::map<std::pair<std::string, int>, int> port_counts;
    countServersByAddress(config, port_counts);
    
    // Vérifier les serveurs avec des server_names en double
    checkDuplicateServerNames(config, port_counts);
    
    // Vérifier chaque configuration de serveur
    validateServerConfigs(config);
}

void ConfigParser::countServersByAddress(const WebservConfig& config, 
                                      std::map<std::pair<std::string, int>, int>& port_counts) {
    for (size_t i = 0; i < config.servers.size(); ++i) {
        const ServerConfig& server = config.servers[i];
        
        // Vérifier la validité de l'adresse IP
        if (server.host != "0.0.0.0" && server.host != "127.0.0.1" && 
            server.host != "localhost") {
            throw std::runtime_error("Invalid host address: " + server.host);
        }

        std::pair<std::string, int> server_addr(server.host, server.port);
        port_counts[server_addr]++;
    }
}

void ConfigParser::checkDuplicateServerNames(const WebservConfig& config, 
                                          const std::map<std::pair<std::string, int>, int>& port_counts) {
    for (std::map<std::pair<std::string, int>, int>::const_iterator port_it = port_counts.begin();
         port_it != port_counts.end(); ++port_it) {
        if (port_it->second > 1) {
            std::vector<const ServerConfig*> servers_on_port;
            
            // Collecter tous les serveurs sur ce port
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
                    std::ostringstream warning;
                    warning << "Server with no server_name on " << server->host << ":" 
                           << server->port << " (using default)";
                    LOG_WARNING(warning.str());
                    name_counts["_default_"]++;
                }
                
                for (size_t j = 0; j < server->server_names.size(); ++j) {
                    name_counts[server->server_names[j]]++;
                }
            }
            
            // Vérifier les doublons
            for (std::map<std::string, int>::const_iterator name_it = name_counts.begin();
                 name_it != name_counts.end(); ++name_it) {
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

void ConfigParser::validateServerConfigs(const WebservConfig& config) {
    for (size_t i = 0; i < config.servers.size(); ++i) {
        const ServerConfig& server = config.servers[i];
        
        // Vérification du port
        if (server.port == 0) {
            throw std::runtime_error("Port is required for each server");
        }
        if (server.port < 1 || server.port > 65535) {
            std::ostringstream error_msg;
            error_msg << "Invalid port number: " << server.port;
            throw std::runtime_error(error_msg.str());
        }
        
        // Vérification des locations
        validateLocations(server);
        
        // Vérification des pages d'erreur
        validateErrorPages(server);
    }
}

void ConfigParser::validateLocations(const ServerConfig& server) {
    std::map<std::string, LocationConfig>::const_iterator loc_it;
    for (loc_it = server.locations.begin(); loc_it != server.locations.end(); ++loc_it) {
        const std::string& path = loc_it->first;
        const LocationConfig& location = loc_it->second;
        
        // Vérifier le chemin de la location
        if (path.empty() || path[0] != '/') {
            throw std::runtime_error("Invalid location path: " + path);
        }
        
        // Vérifier les méthodes autorisées
        if (location.allowed_methods.empty()) {
            throw std::runtime_error("No allowed methods specified for location: " + path);
        }
        
        // Vérifier la validité des méthodes HTTP
        for (size_t i = 0; i < location.allowed_methods.size(); ++i) {
            const std::string& method = location.allowed_methods[i];
            if (method != "GET" && method != "POST" && method != "PUT" && 
                method != "DELETE" && method != "HEAD") {
                throw std::runtime_error("Invalid HTTP method: " + method);
            }
        }
        
        // Vérifier la redirection
        if (location.redirect_code != 0) {
            if (location.redirect_code < 300 || location.redirect_code > 308) {
                std::ostringstream error_msg;
                error_msg << "Invalid redirect code: " << location.redirect_code;
                throw std::runtime_error(error_msg.str());
            }
            if (location.redirect_url.empty()) {
                throw std::runtime_error("Redirect URL required for redirect in: " + path);
            }
        }
        
        // Vérifier les répertoires
        validateLocationDirectories(location);
    }
}

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

void ConfigParser::validateErrorPages(const ServerConfig& server) {
    std::map<int, std::string>::const_iterator err_it;
    for (err_it = server.error_pages.begin(); err_it != server.error_pages.end(); ++err_it) {
        // Vérifier le code d'erreur
        int code = err_it->first;
        if (code < 400 || code > 599) {
            std::ostringstream error_msg;
            error_msg << "Invalid error code: " << code;
            throw std::runtime_error(error_msg.str());
        }
        
        // Vérifier le chemin de la page
        std::string page_path = server.root_directory + "/" + err_it->second;
        if (!fileExists(page_path)) {
            LOG_WARNING("Error page doesn't exist: " << page_path);
        }
    }
} 