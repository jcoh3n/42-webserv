## ÉTAPE 6 - PARSING DU FICHIER DE CONFIGURATION

### Objectif
Implémenter un parseur robuste pour les fichiers de configuration de style NGINX.

### Fonctionnalités clés
- Parser les blocs server et location
- Gérer les directives imbriquées
- Valider la configuration

### Détails techniques

1. **Structure de données pour la configuration**:
```cpp
struct LocationConfig {
    std::string path;
    std::vector<std::string> allowed_methods;
    bool directory_listing;
    std::string index_file;
    std::string redirect_url;
    std::string root_override;
    std::string cgi_extension;
    std::string upload_directory;
    
    LocationConfig() 
        : directory_listing(false)
        , index_file("index.html") {}
};

struct ServerConfig {
    std::string host;
    int port;
    std::vector<std::string> server_names;
    std::string root_directory;
    std::map<std::string, std::string> error_pages;
    size_t client_max_body_size;
    std::map<std::string, LocationConfig> locations;
    
    ServerConfig() 
        : host("0.0.0.0")
        , port(8080)
        , root_directory("/var/www/html")
        , client_max_body_size(1024 * 1024) {} // 1MB par défaut
};

struct WebservConfig {
    std::vector<ServerConfig> servers;
};

Parsing du fichier de configuration:

cppCopierclass ConfigParser {
public:
    WebservConfig parseFile(const std::string& filename) {
        WebservConfig config;
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open config file: " + filename);
        }
        
        std::string line;
        std::vector<std::string> tokens;
        ParsingContext context;
        ServerConfig current_server;
        LocationConfig current_location;
        
        while (std::getline(file, line)) {
            // Supprimer les commentaires
            size_t comment_pos = line.find('#');
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }
            
            // Supprimer les espaces de début et fin
            line = trim(line);
            if (line.empty()) {
                continue;
            }
            
            // Tokenize la ligne
            tokens = tokenizeLine(line);
            if (tokens.empty()) {
                continue;
            }
            
            if (tokens[0] == "server" && tokens.size() >= 2 && tokens[1] == "{") {
                // Début d'un bloc server
                if (context.in_server) {
                    throw std::runtime_error("Nested server blocks not allowed");
                }
                context.in_server = true;
                current_server = ServerConfig();
            }
            else if (tokens[0] == "location" && tokens.size() >= 3 && tokens[2] == "{") {
                // Début d'un bloc location
                if (!context.in_server) {
                    throw std::runtime_error("Location block outside of server block");
                }
                if (context.in_location) {
                    throw std::runtime_error("Nested location blocks not allowed");
                }
                context.in_location = true;
                current_location = LocationConfig();
                current_location.path = tokens[1];
            }
            else if (tokens[0] == "}" && tokens.size() == 1) {
                // Fin d'un bloc
                if (context.in_location) {
                    current_server.locations[current_location.path] = current_location;
                    context.in_location = false;
                }
                else if (context.in_server) {
                    config.servers.push_back(current_server);
                    context.in_server = false;
                }
                else {
                    throw std::runtime_error("Unexpected closing brace");
                }
            }
            else {
                // Directive
                if (context.in_location) {
                    parseLocationDirective(tokens, current_location);
                }
                else if (context.in_server) {
                    parseServerDirective(tokens, current_server);
                }
                else {
                    throw std::runtime_error("Directive outside of server or location block: " + tokens[0]);
                }
            }
        }
        
        // Vérifier que tous les blocs sont fermés
        if (context.in_server || context.in_location) {
            throw std::runtime_error("Unclosed blocks in configuration file");
        }
        
        // Valider la configuration
        validateConfig(config);
        
        return config;
    }

private:
    struct ParsingContext {
        bool in_server;
        bool in_location;
        
        ParsingContext() : in_server(false), in_location(false) {}
    };
    
    std::vector<std::string> tokenizeLine(const std::string& line) {
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string token;
        
        while (iss >> token) {
            // Gérer les tokens spéciaux (accolades, point-virgule)
            if (token.back() == ';') {
                tokens.push_back(token.substr(0, token.size() - 1));
                tokens.push_back(";");
            }
            else if (token.back() == '{') {
                if (token.size() > 1) {
                    tokens.push_back(token.substr(0, token.size() - 1));
                }
                tokens.push_back("{");
            }
            else if (token.back() == '}') {
                if (token.size() > 1) {
                    tokens.push_back(token.substr(0, token.size() - 1));
                }
                tokens.push_back("}");
            }
            else {
                tokens.push_back(token);
            }
        }
        
        return tokens;
    }
    
    void parseServerDirective(const std::vector<std::string>& tokens, ServerConfig& server) {
        if (tokens.size() < 2 || tokens.back() != ";") {
            throw std::runtime_error("Invalid directive syntax: " + tokens[0]);
        }
        
        std::string directive = tokens[0];
        
        if (directive == "listen") {
            parseListen(tokens, server);
        }
        else if (directive == "server_name") {
            parseServerName(tokens, server);
        }
        else if (directive == "root") {
            server.root_directory = tokens[1];
        }
        else if (directive == "client_max_body_size") {
            parseBodySize(tokens, server);
        }
        else if (directive == "error_page") {
            parseErrorPage(tokens, server);
        }
        else {
            throw std::runtime_error("Unknown server directive: " + directive);
        }
    }
    
    void parseLocationDirective(const std::vector<std::string>& tokens, LocationConfig& location) {
        if (tokens.size() < 2 || tokens.back() != ";") {
            throw std::runtime_error("Invalid directive syntax: " + tokens[0]);
        }
        
        std::string directive = tokens[0];
        
        if (directive == "methods") {
            parseMethods(tokens, location);
        }
        else if (directive == "root") {
            location.root_override = tokens[1];
        }
        else if (directive == "index") {
            location.index_file = tokens[1];
        }
        else if (directive == "autoindex") {
            location.directory_listing = (tokens[1] == "on");
        }
        else if (directive == "return") {
            location.redirect_url = tokens[1];
        }
        else if (directive == "cgi_extension") {
            location.cgi_extension = tokens[1];
        }
        else if (directive == "upload_dir") {
            location.upload_directory = tokens[1];
        }
        else {
            throw std::runtime_error("Unknown location directive: " + directive);
        }
    }
    
    void parseListen(const std::vector<std::string>& tokens, ServerConfig& server) {
        std::string value = tokens[1];
        
        // Possible formats: 8080, localhost:8080, 127.0.0.1:8080
        size_t colon_pos = value.find(':');
        if (colon_pos != std::string::npos) {
            server.host = value.substr(0, colon_pos);
            server.port = std::stoi(value.substr(colon_pos + 1));
        }
        else {
            try {
                server.port = std::stoi(value);
            }
            catch (const std::exception& e) {
                server.host = value;
                server.port = 80; // Port par défaut pour HTTP
            }
        }
    }
    
    void parseServerName(const std::vector<std::string>& tokens, ServerConfig& server) {
        for (size_t i = 1; i < tokens.size() - 1; ++i) {
            server.server_names.push_back(tokens[i]);
        }
    }
    
    void parseBodySize(const std::vector<std::string>& tokens, ServerConfig& server) {
        std::string value = tokens[1];
        server.client_max_body_size = parseSize(value);
    }
    
    void parseErrorPage(const std::vector<std::string>& tokens, ServerConfig& server) {
        if (tokens.size() < 4) {
            throw std::runtime_error("Invalid error_page directive");
        }
        
        std::string error_code = tokens[1];
        std::string page_path = tokens[2];
        
        server.error_pages[error_code] = page_path;
    }
    
    void parseMethods(const std::vector<std::string>& tokens, LocationConfig& location) {
        for (size_t i = 1; i < tokens.size() - 1; ++i) {
            std::string method = tokens[i];
            std::transform(method.begin(), method.end(), method.begin(), ::toupper);
            location.allowed_methods.push_back(method);
        }
    }
    
    size_t parseSize(const std::string& size_str) {
        size_t len = size_str.length();
        char unit = size_str[len - 1];
        
        if (std::isdigit(unit)) {
            return std::stoul(size_str);
        }
        
        size_t value = std::stoul(size_str.substr(0, len - 1));
        
        switch (unit) {
            case 'K':
            case 'k':
                return value * 1024;
            case 'M':
            case 'm':
                return value * 1024 * 1024;
            case 'G':
            case 'g':
                return value * 1024 * 1024 * 1024;
            default:
                throw std::runtime_error("Invalid size unit: " + std::string(1, unit));
        }
    }
    
    void validateConfig(const WebservConfig& config) {
        if (config.servers.empty()) {
            throw std::runtime_error("No server blocks found");
        }
        
        // Vérifier les conflits de port
        std::map<std::pair<std::string, int>, int> port_counts;
        for (const auto& server : config.servers) {
            port_counts[std::make_pair(server.host, server.port)]++;
        }
        
        // Plusieurs serveurs peuvent écouter sur le même port s'ils ont des server_names différents
        for (const auto& count : port_counts) {
            if (count.second > 1) {
                // Vérifier que les serveurs ont des server_names différents
                std::vector<ServerConfig> servers_on_port;
                for (const auto& server : config.servers) {
                    if (server.host == count.first.first && server.port == count.first.second) {
                        servers_on_port.push_back(server);
                    }
                }
                
                // Vérifier les conflits de server_name
                std::map<std::string, int> name_counts;
                for (const auto& server : servers_on_port) {
                    if (server.server_names.empty()) {
                        name_counts["_default_"]++;
                    }
                    for (const auto& name : server.server_names) {
                        name_counts[name]++;
                    }
                }
                
                for (const auto& name_count : name_counts) {
                    if (name_count.second > 1) {
                        throw std::runtime_error("Duplicate server_name '" + name_count.first + 
                                               "' for " + count.first.first + ":" + 
                                               std::to_string(count.first.second));
                    }
                }
            }
        }
        
        // Valider chaque serveur
        for (const auto& server : config.servers) {
            // Vérifier les routes en conflit
            std::map<std::string, int> location_counts;
            for (const auto& location : server.locations) {
                location_counts[location.first]++;
                if (location_counts[location.first] > 1) {
                    throw std::runtime_error("Duplicate location '" + location.first + 
                                           "' in server " + server.host + ":" + 
                                           std::to_string(server.port));
                }
                
                // Vérifier que les chemins existent
                if (!location.second.root_override.empty()) {
                    if (!directoryExists(location.second.root_override)) {
                        throw std::runtime_error("Root directory doesn't exist: " + 
                                               location.second.root_override);
                    }
                }
                
                if (!location.second.upload_directory.empty()) {
                    if (!directoryExists(location.second.upload_directory)) {
                        throw std::runtime_error("Upload directory doesn't exist: " + 
                                               location.second.upload_directory);
                    }
                }
            }
            
            // Vérifier que le root directory existe
            if (!directoryExists(server.root_directory)) {
                throw std::runtime_error("Root directory doesn't exist: " + server.root_directory);
            }
            
            // Vérifier que les pages d'erreur existent
            for (const auto& error_page : server.error_pages) {
                std::string page_path = server.root_directory + "/" + error_page.second;
                if (!fileExists(page_path)) {
                    throw std::runtime_error("Error page doesn't exist: " + page_path);
                }
            }
        }
    }
    
    bool directoryExists(const std::string& path) {
        struct stat info;
        if (stat(path.c_str(), &info) != 0) {
            return false;
        }
        return (info.st_mode & S_IFDIR) != 0;
    }
    
    bool fileExists(const std::string& path) {
        struct stat info;
        if (stat(path.c_str(), &info) != 0) {
            return false;
        }
        return (info.st_mode & S_IFREG) != 0;
    }
    
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r\f\v");
        if (first == std::string::npos) {
            return "";
        }
        size_t last = str.find_last_not_of(" \t\n\r\f\v");
        return str.substr(first, last - first + 1);
    }
};

Sélection du serveur virtuel approprié:

cppCopierconst ServerConfig& selectVirtualServer(const WebservConfig& config, 
                                       const std::string& host, 
                                       int port) {
    // Chercher les serveurs qui correspondent au port
    std::vector<const ServerConfig*> matching_servers;
    
    for (const auto& server : config.servers) {
        if (server.port == port) {
            // Si l'hôte correspond exactement, c'est une correspondance parfaite
            if (server.host == host) {
                matching_servers.push_back(&server);
            }
            // Si l'hôte du serveur est 0.0.0.0, il accepte toutes les adresses
            else if (server.host == "0.0.0.0") {
                matching_servers.push_back(&server);
            }
        }
    }
    
    if (matching_servers.empty()) {
        throw std::runtime_error("No server configured for " + host + ":" + std::to_string(port));
    }
    
    // S'il n'y a qu'un seul serveur correspondant, c'est facile
    if (matching_servers.size() == 1) {
        return *matching_servers[0];
    }
    
    // Extraire le nom d'hôte à partir de l'en-tête Host
    std::string hostname = host;
    size_t colon_pos = hostname.find(':');
    if (colon_pos != std::string::npos) {
        hostname = hostname.substr(0, colon_pos);
    }
    
    // Chercher un serveur avec un server_name correspondant
    for (const auto* server : matching_servers) {
        for (const auto& server_name : server->server_names) {
            if (server_name == hostname) {
                return *server;
            }
        }
    }
    
    // Pas de correspondance exacte par server_name, retourner le premier serveur
    // (qui sera le serveur par défaut pour cette IP:port)
    return *matching_servers[0];
}

Sélection de la location appropriée:

cppCopierconst LocationConfig& selectLocation(const ServerConfig& server, const std::string& uri) {
    // Normaliser l'URI (supprimer la query string)
    std::string normalized_uri = uri;
    size_t query_pos = normalized_uri.find('?');
    if (query_pos != std::string::npos) {
        normalized_uri = normalized_uri.substr(0, query_pos);
    }
    
    // Trouver la correspondance la plus longue
    std::string best_match = "";
    const LocationConfig* best_location = nullptr;
    
    for (const auto& location_pair : server.locations) {
        const std::string& location_path = location_pair.first;
        
        // Correspondance exacte
        if (normalized_uri == location_path) {
            return location_pair.second;
        }
        
        // Correspondance de préfixe
        if (normalized_uri.find(location_path) == 0 && 
            (location_path.length() > best_match.length())) {
            best_match = location_path;
            best_location = &location_pair.second;
        }
    }
    
    // Si une correspondance a été trouvée
    if (best_location) {
        return *best_location;
    }
    
    // Pas de correspondance, vérifier s'il y a une location "/"
    auto root_location = server.locations.find("/");
    if (root_location != server.locations.end()) {
        return root_location->second;
    }
    
    // Pas de location configurée, utiliser une configuration par défaut
    static LocationConfig default_location;
    return default_location;
}
Points de validation

 Parse correctement un fichier de configuration basique
 Gère plusieurs blocs server
 Interprète correctement les directives communes (listen, server_name, root)
 Valide les valeurs des directives
 Détecte les erreurs de syntaxe dans le fichier de configuration
 Sélectionne correctement le serveur virtuel et la location
 Gère correctement les conflits (port, server_name)

Tests recommandés

Test de configuration valide:

bashCopier./webserv tests/valid_config.conf

Test avec syntaxe incorrecte:

bashCopier./webserv tests/invalid_syntax.conf

Test avec configuration invalide (valeurs incorrectes):

bashCopier./webserv tests/invalid_values.conf

Test avec plusieurs serveurs virtuels:

bashCopier./webserv tests/virtual_hosts.conf
# Puis tester avec des noms d'hôte différents
curl -H "Host: site1.local" http://localhost:8080/
curl -H "Host: site2.local" http://localhost:8080/

Test avec différentes locations:

bashCopier./webserv tests/locations.conf
curl http://localhost:8080/api/
curl http://localhost:8080/static/
Bonnes pratiques

Vérifier l'existence des chemins spécifiés
Utiliser des valeurs par défaut raisonnables
Journaliser les problèmes de configuration clairement
Valider rigoureusement la syntaxe et la sémantique
Normaliser les chemins et les URIs
Implémenter une sélection de serveur virtuel conforme à HTTP/1.1