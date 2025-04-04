#include "config/ConfigParser.hpp"
#include "config/ConfigTypes.hpp"
#include <algorithm>
#include <sstream>

const ServerConfig& selectVirtualServer(const WebservConfig& config, 
                                       const std::string& hostname, 
                                       int port) {
    std::vector<const ServerConfig*> matching_servers;
    findServersByPort(config, port, matching_servers);
    
    if (matching_servers.empty()) {
        std::ostringstream error_msg;
        error_msg << "No server matching " << hostname << ":" << port;
        throw std::runtime_error(error_msg.str());
    }
    
    // On cherche d'abord un serveur avec un nom correspondant
    const ServerConfig* server = findServerByName(matching_servers, hostname);
    
    if (server) {
        return *server;
    }
    
    // Si aucun nom ne correspond, on prend le premier serveur sur ce port
    return *matching_servers[0];
}

void findServersByPort(const WebservConfig& config, int port, 
                     std::vector<const ServerConfig*>& matching_servers) {
    matching_servers.clear();
    
    for (size_t i = 0; i < config.servers.size(); ++i) {
        const ServerConfig& server = config.servers[i];
        
        if (server.port == port) {
            matching_servers.push_back(&server);
        }
    }
}

const ServerConfig* findServerByName(const std::vector<const ServerConfig*>& servers, 
                                   const std::string& hostname) {
    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfig* server = servers[i];
        
        for (size_t j = 0; j < server->server_names.size(); ++j) {
            if (server->server_names[j] == hostname) {
                return server;
            }
        }
    }
    
    return NULL;
}

const LocationConfig& selectLocation(const ServerConfig& server, const std::string& uri) {
    const LocationConfig* loc = findBestLocationMatch(server, uri);
    
    if (!loc) {
        // Si aucune location ne correspond, on utilise la location racine (/) ou on lève une exception
        std::map<std::string, LocationConfig>::const_iterator root_loc = server.locations.find("/");
        if (root_loc != server.locations.end()) {
            return root_loc->second;
        } else {
            throw std::runtime_error("No matching location for URI: " + uri);
        }
    }
    
    return *loc;
}

const LocationConfig* findBestLocationMatch(const ServerConfig& server, const std::string& uri) {
    const LocationConfig* best_match = NULL;
    size_t best_match_len = 0;
    
    std::map<std::string, LocationConfig>::const_iterator loc_it;
    for (loc_it = server.locations.begin(); loc_it != server.locations.end(); ++loc_it) {
        std::string loc_path = loc_it->first;
        
        // Si le chemin correspond exactement
        if (uri == loc_path) {
            return &loc_it->second;
        }
        
        // Si c'est un préfixe
        if (isLocationPrefixMatch(uri, loc_path)) {
            size_t path_len = loc_path.length();
            if (path_len > best_match_len) {
                best_match = &loc_it->second;
                best_match_len = path_len;
            }
        }
    }
    
    return best_match;
}

bool isLocationPrefixMatch(const std::string& uri, const std::string& location_path) {
    // Si le location_path est /, il correspond à tout
    if (location_path == "/") {
        return true;
    }
    
    // Sinon, il doit être un préfixe exact ou suivi d'un /
    if (uri.compare(0, location_path.length(), location_path) == 0) {
        return uri.length() == location_path.length() || 
               uri[location_path.length()] == '/' ||
               location_path[location_path.length() - 1] == '/';
    }
    
    return false;
} 