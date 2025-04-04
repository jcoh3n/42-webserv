#include "config/ConfigParser.hpp"
#include "utils/Common.hpp"
#include <cctype>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <algorithm>

WebservConfig ConfigParser::parseFile(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + filename);
    }

    WebservConfig config;
    ServerConfig current_server;
    LocationConfig current_location;
    std::string section = "";
    std::string location_path = "";
    int line_number = 0;
    int brace_level = 0;
    bool in_server = false;
    bool in_location = false;

    std::string line;
    try {
        while (std::getline(file, line)) {
            line_number++;
            
            // Ignorer les lignes vides et les commentaires
            line = trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // Traitement des accolades et des directives
            parseNewSyntax(line, config, current_server, current_location, 
                        location_path, in_server, in_location, brace_level);
        }
        
        // Vérifier que toutes les accolades sont fermées
        if (brace_level != 0) {
            throw std::runtime_error("Unclosed braces in config file");
        }
        
        // Ajouter le dernier serveur s'il n'a pas été ajouté
        if (in_server) {
            config.servers.push_back(current_server);
        }
    } catch (const std::exception& e) {
        std::ostringstream errorMsg;
        errorMsg << "Error on line " << line_number << ": " << e.what();
        throw std::runtime_error(errorMsg.str());
    }
    
    if (config.servers.empty()) {
        throw std::runtime_error("No server section defined in config file");
    }

    validateConfig(config);
    return config;
}

void ConfigParser::parseNewSyntax(const std::string& line, WebservConfig& config,
                               ServerConfig& current_server, LocationConfig& current_location,
                               std::string& location_path, bool& in_server, bool& in_location,
                               int& brace_level) {
    // Extraction des tokens principaux (avant les commentaires)
    std::string content = line;
    size_t comment_pos = content.find('#');
    if (comment_pos != std::string::npos) {
        content = content.substr(0, comment_pos);
        content = trim(content);
        if (content.empty()) return;
    }
    
    // Détection d'ouverture de bloc serveur: "server {"
    if (content.find("server") == 0 && content.find("{") != std::string::npos) {
        if (in_server) {
            // Finir le serveur précédent avant d'en commencer un nouveau
            config.servers.push_back(current_server);
        }
        in_server = true;
        in_location = false;
        current_server = ServerConfig();
        brace_level++;
        return;
    }
    
    // Détection d'ouverture de bloc location: "location /path {"
    if (content.find("location") == 0) {
        if (!in_server) {
            throw std::runtime_error("Location block must be inside a server block");
        }
        
        if (in_location) {
            // Finir la location précédente
            current_server.locations[location_path] = current_location;
        }
        
        in_location = true;
        
        // Extraire le chemin de la location
        std::istringstream iss(content);
        std::string dummy, path;
        iss >> dummy >> path;
        
        // Vérifier la présence de l'accolade ouvrante
        if (content.find("{") == std::string::npos) {
            throw std::runtime_error("Missing opening brace for location block");
        }
        
        location_path = path;
        current_location = LocationConfig();
        brace_level++;
        return;
    }
    
    // Détection de fermeture de bloc avec "}"
    if (content.find("}") != std::string::npos) {
        brace_level--;
        
        if (in_location) {
            // Fermeture d'un bloc location
            current_server.locations[location_path] = current_location;
            in_location = false;
        } else if (in_server && brace_level == 0) {
            // Fermeture d'un bloc server
            config.servers.push_back(current_server);
            in_server = false;
        }
        return;
    }
    
    // Traitement des directives (clé=valeur ou clé valeur)
    parseDirective(content, in_server, in_location, current_server, current_location);
}

void ConfigParser::parseDirective(const std::string& line, bool in_server, bool in_location,
                               ServerConfig& server, LocationConfig& location) {
    // Format flexible: clé=valeur ou clé valeur
    std::string key, value;
    size_t pos = line.find('=');
    
    if (pos != std::string::npos) {
        // Format clé=valeur
        key = trim(line.substr(0, pos));
        value = trim(line.substr(pos + 1));
    } else {
        // Format clé valeur
        std::istringstream iss(line);
        iss >> key;
        
        // Le reste de la ligne est la valeur
        std::getline(iss, value);
        value = trim(value);
    }
    
    if (key.empty()) {
        return; // Ligne vide ou mal formatée
    }
    
    // Traiter la directive selon le contexte
    if (in_location) {
        processLocationDirective(key, value, location);
    } else if (in_server) {
        processServerDirective(key, value, server);
    } else {
        throw std::runtime_error("Directive outside of server or location block: " + key);
    }
}

void ConfigParser::processServerDirective(const std::string& key, const std::string& value,
                                       ServerConfig& server) {
    if (key == "port") {
        int port = atoi(value.c_str());
        if (port <= 0 || port > 65535) {
            throw std::runtime_error("Invalid port number (must be between 1 and 65535)");
        }
        server.port = port;
    } else if (key == "host") {
        server.host = value;
    } else if (key == "server_name") {
        server.server_names = split(value, ' ');
    } else if (key == "root") {
        server.root_directory = value;
    } else if (key == "index") {
        server.index_files = split(value, ' ');
    } else if (key == "error_page") {
        std::vector<std::string> parts = split(value, ' ');
        if (parts.size() < 2) {
            throw std::runtime_error("Invalid error_page format (should be: error_page=code file_path)");
        }
        int code = atoi(parts[0].c_str());
        server.error_pages[code] = parts[1];
    } else {
        throw std::runtime_error("Unknown server directive: " + key);
    }
}

void ConfigParser::processLocationDirective(const std::string& key, const std::string& value,
                                         LocationConfig& location) {
    if (key == "allowed_methods") {
        location.allowed_methods = split(value, ' ');
    } else if (key == "root") {
        location.root_override = value;
    } else if (key == "index") {
        location.index_files = split(value, ' ');
    } else if (key == "autoindex") {
        location.autoindex = (value == "on" || value == "true" || value == "1");
    } else if (key == "upload_directory") {
        location.upload_directory = value;
    } else if (key == "return") {
        std::vector<std::string> parts = split(value, ' ');
        if (parts.size() < 2) {
            throw std::runtime_error("Invalid return format (should be: return=code url)");
        }
        location.redirect_code = atoi(parts[0].c_str());
        location.redirect_url = parts[1];
    } else if (key == "cgi") {
        location.cgi_extensions = split(value, ' ');
    } else if (key == "alias") {
        location.alias = value;
    } else if (key == "client_max_body_size") {
        location.client_max_body_size = parseSize(value);
        if (location.client_max_body_size == 0) {
            throw std::runtime_error("client_max_body_size must be greater than 0");
        }
    } else {
        throw std::runtime_error("Unknown location directive: " + key);
    }
}



