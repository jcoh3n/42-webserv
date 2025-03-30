## ÉTAPE 5 - GESTION DES FICHIERS STATIQUES

### Objectif
Implémenter la capacité à servir des fichiers statiques (HTML, CSS, JS, images) depuis le système de fichiers de manière sécurisée et optimisée.

### Fonctionnalités clés
- Servir des fichiers depuis un dossier racine configurable
- Gérer les types MIME correctement
- Prendre en charge les fichiers binaires (images)
- Gérer les permissions et erreurs
- Protection contre les attaques de traversée de chemin
- Support des fonctionnalités de cache

### Détails techniques

1. **Structure de configuration**:
```cpp
struct StaticFileConfig {
    std::string root_directory;      // Chemin de base (ex: "/var/www")
    std::string default_file;        // Fichier par défaut (ex: "index.html")
    bool directory_listing;          // Autoriser l'affichage des répertoires
    size_t max_file_size;            // Taille maximale des fichiers à servir
    std::map<std::string, std::string> error_pages; // Pages d'erreur personnalisées
};

Vérification sécurisée des chemins:

cppCopierstd::string resolveFilePath(const std::string& uri, const StaticFileConfig& config) {
    // Normaliser l'URI (supprimer les '//' consécutifs, etc.)
    std::string normalized_uri = uri;
    size_t pos = 0;
    while ((pos = normalized_uri.find("//", pos)) != std::string::npos) {
        normalized_uri.replace(pos, 2, "/");
    }
    
    // Supprimer les '/' au début et à la fin si présents
    if (!normalized_uri.empty() && normalized_uri.front() == '/') {
        normalized_uri = normalized_uri.substr(1);
    }
    
    // Construire le chemin complet
    std::string path = config.root_directory;
    if (!path.empty() && path.back() != '/') {
        path += '/';
    }
    path += normalized_uri;
    
    // Vérifier la traversée de répertoire (../../../etc/passwd)
    char resolved_path[PATH_MAX];
    if (realpath(path.c_str(), resolved_path) == NULL) {
        // Le chemin n'existe pas ou erreur
        return "";
    }
    
    // Vérifier que le chemin résolu commence bien par le répertoire racine
    if (strncmp(resolved_path, config.root_directory.c_str(), config.root_directory.size()) != 0) {
        // Tentative de sortir du répertoire racine
        return "";
    }
    
    return resolved_path;
}

Vérification du type de fichier:

cppCopierbool isDirectory(const std::string& path) {
    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) != 0) {
        return false;
    }
    return S_ISDIR(path_stat.st_mode);
}

bool isRegularFile(const std::string& path) {
    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) != 0) {
        return false;
    }
    return S_ISREG(path_stat.st_mode);
}

Lecture sécurisée des fichiers:

cppCopierstd::string readFileContent(const std::string& file_path, size_t max_size) {
    // Vérifier la taille du fichier avant de le lire
    struct stat file_stat;
    if (stat(file_path.c_str(), &file_stat) != 0) {
        throw std::runtime_error("Cannot stat file: " + std::string(strerror(errno)));
    }
    
    if (static_cast<size_t>(file_stat.st_size) > max_size) {
        throw std::runtime_error("File size exceeds limit");
    }
    
    // Ouvrir le fichier
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + std::string(strerror(errno)));
    }

    // Lire le contenu
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("Error reading file: " + std::string(strerror(errno)));
    }

    return std::string(buffer.data(), size);
}

Génération du listing de répertoire:

cppCopierstd::string generateDirectoryListing(const std::string& path, const std::string& uri) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        throw std::runtime_error("Cannot open directory: " + std::string(strerror(errno)));
    }

    std::stringstream listing;
    listing << "<!DOCTYPE html>\n"
            << "<html>\n"
            << "<head>\n"
            << "    <title>Index of " << uri << "</title>\n"
            << "    <style>\n"
            << "        body { font-family: Arial, sans-serif; margin: 20px; }\n"
            << "        h1 { border-bottom: 1px solid #ccc; padding-bottom: 10px; }\n"
            << "        table { width: 100%; border-collapse: collapse; }\n"
            << "        th { text-align: left; padding: 8px; background-color: #f2f2f2; }\n"
            << "        td { padding: 8px; border-top: 1px solid #ddd; }\n"
            << "        tr:hover { background-color: #f5f5f5; }\n"
            << "        a { text-decoration: none; color: #0066cc; }\n"
            << "    </style>\n"
            << "</head>\n"
            << "<body>\n"
            << "    <h1>Index of " << uri << "</h1>\n"
            << "    <table>\n"
            << "        <tr>\n"
            << "            <th>Name</th>\n"
            << "            <th>Last Modified</th>\n"
            << "            <th>Size</th>\n"
            << "        </tr>\n";
    
    // Ajouter un lien vers le répertoire parent
    if (uri != "/") {
        listing << "        <tr>\n"
                << "            <td><a href=\"..\">..</a></td>\n"
                << "            <td></td>\n"
                << "            <td></td>\n"
                << "        </tr>\n";
    }

    std::vector<dirent*> entries;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        entries.push_back(entry);
    }
    
    // Trier les entrées (dossiers d'abord, puis fichiers)
    std::sort(entries.begin(), entries.end(), [&path](const dirent* a, const dirent* b) {
        bool a_is_dir = isDirectory(path + "/" + a->d_name);
        bool b_is_dir = isDirectory(path + "/" + b->d_name);
        
        if (a_is_dir && !b_is_dir) return true;
        if (!a_is_dir && b_is_dir) return false;
        
        return strcasecmp(a->d_name, b->d_name) < 0;
    });
    
    // Ajouter chaque entrée
    for (const auto& entry : entries) {
        std::string entry_name = entry->d_name;
        std::string entry_path = path + "/" + entry_name;
        
        struct stat file_stat;
        if (stat(entry_path.c_str(), &file_stat) != 0) {
            continue;
        }
        
        bool is_dir = S_ISDIR(file_stat.st_mode);
        std::string entry_link = entry_name;
        if (is_dir) {
            entry_link += "/";
        }
        
        listing << "        <tr>\n"
                << "            <td><a href=\"" << entry_link << "\">" << entry_name << (is_dir ? "/" : "") << "</a></td>\n"
                << "            <td>" << std::put_time(std::localtime(&file_stat.st_mtime), "%Y-%m-%d %H:%M:%S") << "</td>\n"
                << "            <td>" << (is_dir ? "-" : std::to_string(file_stat.st_size) + " bytes") << "</td>\n"
                << "        </tr>\n";
    }
    
    listing << "    </table>\n"
            << "    <hr>\n"
            << "    <p>webserv/" << VERSION << " Server</p>\n"
            << "</body>\n"
            << "</html>";
    
    closedir(dir);
    return listing.str();
}

Gestion des requêtes de fichiers statiques:

cppCopierHttpResponse handleStaticFileRequest(const HttpRequest& request, const StaticFileConfig& config) {
    HttpResponse response;
    std::string uri = request.uri;
    
    // Vérifier le chemin et résoudre les redirections
    if (uri.find("..") != std::string::npos) {
        return createErrorResponse(403, "Forbidden");
    }
    
    // Si l'URI pointe vers un répertoire mais ne se termine pas par '/', rediriger
    std::string file_path = resolveFilePath(uri, config);
    if (file_path.empty()) {
        return serveErrorPage(404, config);
    }
    
    if (isDirectory(file_path)) {
        // Si l'URI ne se termine pas par '/', rediriger avec un '/'
        if (uri.empty() || uri[uri.size() - 1] != '/') {
            response.setRedirect(uri + "/", 301);
            return response;
        }
        
        // Essayer le fichier par défaut (index.html)
        std::string index_path = file_path + "/" + config.default_file;
        if (isRegularFile(index_path)) {
            file_path = index_path;
        } else if (config.directory_listing) {
            // Générer une liste du répertoire
            try {
                std::string listing = generateDirectoryListing(file_path, uri);
                response.setStatus(200);
                response.setBody(listing);
                return response;
            } catch (const std::exception& e) {
                return serveErrorPage(500, config);
            }
        } else {
            // Directory listing désactivé
            return serveErrorPage(403, config);
        }
    }
    
    // Vérifier si le fichier existe et est accessible
    if (!isRegularFile(file_path)) {
        return serveErrorPage(404, config);
    }
    
    // Vérifier les permissions
    if (access(file_path.c_str(), R_OK) != 0) {
        return serveErrorPage(403, config);
    }
    
    // Vérifier si le fichier n'a pas été modifié (ETag)
    if (checkNotModified(request, file_path, response)) {
        return response;
    }
    
    // Déterminer le type MIME
    std::string mime_type = getMimeType(file_path);
    
    // Servir le fichier
    try {
        // Pour les gros fichiers, utiliser un streaming plutôt que de charger tout en mémoire
        struct stat file_stat;
        if (stat(file_path.c_str(), &file_stat) == 0 && 
            static_cast<size_t>(file_stat.st_size) > LARGE_FILE_THRESHOLD) {
            // Marquer la réponse pour un traitement spécial (streaming)
            response.setStatus(200);
            response.setHeader("Content-Type", mime_type);
            response.large_file_path = file_path;
            return response;
        }
        
        // Pour les fichiers normaux, les lire complètement
        std::string content = readFileContent(file_path, config.max_file_size);
        response.setStatus(200);
        response.setBody(content, mime_type);
        
        // Ajouter des headers de cache
        time_t mtime = file_stat.st_mtime;
        char last_modified[100];
        strftime(last_modified, sizeof(last_modified), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&mtime));
        response.setHeader("Last-Modified", last_modified);
        
        // Par défaut, cache client d'une heure pour les fichiers statiques
        response.setHeader("Cache-Control", "public, max-age=3600");
        
    } catch (const std::exception& e) {
        return serveErrorPage(500, config);
    }
    
    return response;
}

Service des pages d'erreur personnalisées:

cppCopierHttpResponse serveErrorPage(int error_code, const StaticFileConfig& config) {
    // Vérifier si une page d'erreur personnalisée est configurée
    auto it = config.error_pages.find(std::to_string(error_code));
    if (it != config.error_pages.end()) {
        std::string error_file = config.root_directory + "/" + it->second;
        if (isRegularFile(error_file) && access(error_file.c_str(), R_OK) == 0) {
            try {
                std::string content = readFileContent(error_file, config.max_file_size);
                HttpResponse response;
                response.setStatus(error_code);
                response.setBody(content, getMimeType(error_file));
                return response;
            } catch (const std::exception& e) {
                // En cas d'erreur, retourner à la page d'erreur par défaut
            }
        }
    }
    
    // Page d'erreur par défaut
    return createErrorResponse(error_code);
}

Optimisation avec Cache-Control et ETag:

cppCopiervoid addCacheHeaders(HttpResponse& response, const std::string& file_path) {
    struct stat file_stat;
    if (stat(file_path.c_str(), &file_stat) != 0) {
        return;
    }
    
    // Last-Modified
    char last_modified[100];
    strftime(last_modified, sizeof(last_modified), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&file_stat.st_mtime));
    response.setHeader("Last-Modified", last_modified);
    
    // ETag (basé sur inode, taille et mtime)
    std::stringstream etag;
    etag << "\"" << file_stat.st_ino << "-" << file_stat.st_size << "-" << file_stat.st_mtime << "\"";
    response.setHeader("ETag", etag.str());
    
    // Cache-Control en fonction du type de fichier
    std::string extension = file_path.substr(file_path.find_last_of(".") + 1);
    if (extension == "html" || extension == "htm") {
        // Pages HTML: cache court (5 min)
        response.setHeader("Cache-Control", "public, max-age=300");
    } else if (extension == "css" || extension == "js") {
        // Ressources statiques: cache plus long (1 jour)
        response.setHeader("Cache-Control", "public, max-age=86400");
    } else if (extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "gif") {
        // Images: cache très long (1 semaine)
        response.setHeader("Cache-Control", "public, max-age=604800");
    } else {
        // Autres fichiers: cache moyen (1 heure)
        response.setHeader("Cache-Control", "public, max-age=3600");
    }
}
Points de validation

 Fichiers HTML/CSS/JS servis correctement
 Images affichées correctement
 Protection contre directory traversal (../../../etc/passwd)
 Gestion correcte des permissions
 Redirection pour les répertoires sans '/'
 Fichier par défaut (index.html) fonctionnel
 Listing de répertoire activable/désactivable
 Pages d'erreur personnalisées
 Cache-Control et ETag corrects
 Gestion des gros fichiers en streaming

Tests recommandés

Test fichier existant:

bashCopiercurl -v http://localhost:8080/index.html

Test image:

bashCopiercurl -v http://localhost:8080/images/logo.png --output test.png

Test répertoire:

bashCopiercurl -v http://localhost:8080/docs/

Test sécurité:

bashCopiercurl -v http://localhost:8080/../../../etc/passwd

Test listing de répertoire:

bashCopiercurl -v http://localhost:8080/docs/

Test ETag:

bashCopier# Premier appel
curl -v http://localhost:8080/index.html
# Second appel avec l'ETag
curl -v -H "If-None-Match: \"12345-67890-1234567890\"" http://localhost:8080/index.html

Test gros fichier:

bashCopier# Créer un fichier de 10MB
dd if=/dev/zero of=/tmp/www/large_file.bin bs=1M count=10
curl -v http://localhost:8080/large_file.bin -o /dev/null
Bonnes pratiques

Toujours résoudre les chemins complètement avant d'y accéder
Valider rigoureusement tous les chemins de fichiers
Limiter la taille des fichiers à servir
Optimiser le transfert des gros fichiers (sendfile ou chunked)
Utiliser des Content-Type précis et corrects
Implémenter un cache client efficace (ETag, Cache-Control)
Journaliser les accès pour détecter les tentatives d'intrusion
Implémenter une solution élégante pour les pages d'erreur