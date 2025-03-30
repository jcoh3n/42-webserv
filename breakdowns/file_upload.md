## ÉTAPE 8 - GESTION DES UPLOADS DE FICHIERS

### Objectif
Implémenter un mécanisme robuste pour permettre aux clients de téléverser des fichiers sur le serveur.

### Fonctionnalités clés
- Parser les requêtes multipart/form-data
- Extraire et sauvegarder les fichiers
- Gérer les limites de taille
- Valider les types de fichiers (optionnel)

### Détails techniques

1. **Configuration pour les uploads**:
```cpp
struct UploadConfig {
    std::string directory;         // Répertoire de destination
    size_t max_file_size;          // Taille maximale par fichier (en octets)
    std::vector<std::string> allowed_extensions;  // Extensions autorisées
};

Parsing des requêtes multipart/form-data:

cppCopierstruct FileUpload {
    std::string filename;
    std::string content_type;
    std::string data;
};

std::vector<FileUpload> parseMultipartFormData(const HttpRequest& request) {
    std::vector<FileUpload> uploads;
    
    // Vérifier que c'est bien une requête multipart/form-data
    auto it = request.headers.find("content-type");
    if (it == request.headers.end() || it->second.find("multipart/form-data") == std::string::npos) {
        return uploads;
    }
    
    // Extraire la boundary
    std::string content_type = it->second;
    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string::npos) {
        return uploads;
    }
    
    std::string boundary = "--" + content_type.substr(boundary_pos + 9);
    
    // Découper le body en parties
    size_t pos = 0;
    while ((pos = request.body.find(boundary, pos)) != std::string::npos) {
        pos += boundary.length();
        
        // Ignorer les sauts de ligne
        if (request.body[pos] == '\r') pos += 2;
        
        // Vérifier si c'est la fin
        if (request.body.substr(pos, 2) == "--") {
            break;
        }
        
        // Trouver la fin des headers de cette partie
        size_t headers_end = request.body.find("\r\n\r\n", pos);
        if (headers_end == std::string::npos) break;
        
        // Parser les headers
        std::string headers_section = request.body.substr(pos, headers_end - pos);
        std::map<std::string, std::string> part_headers;
        
        std::istringstream headers_stream(headers_section);
        std::string header_line;
        while (std::getline(headers_stream, header_line)) {
            if (header_line.empty() || header_line == "\r") continue;
            
            // Supprimer le \r à la fin
            if (!header_line.empty() && header_line.back() == '\r') {
                header_line.pop_back();
            }
            
            size_t colon_pos = header_line.find(":");
            if (colon_pos != std::string::npos) {
                std::string name = header_line.substr(0, colon_pos);
                std::string value = header_line.substr(colon_pos + 1);
                
                // Supprimer les espaces en début/fin
                value.erase(0, value.find_first_not_of(" \t"));
                part_headers[name] = value;
            }
        }
        
        // Vérifier si c'est un fichier
        auto content_disp_it = part_headers.find("Content-Disposition");
        if (content_disp_it != part_headers.end() && 
            content_disp_it->second.find("filename=") != std::string::npos) {
            
            FileUpload upload;
            
            // Extraire le nom du fichier
            std::string filename_part = content_disp_it->second;
            size_t filename_start = filename_part.find("filename=\"") + 10;
            size_t filename_end = filename_part.find("\"", filename_start);
            upload.filename = filename_part.substr(filename_start, filename_end - filename_start);
            
            // Extraire le type de contenu
            auto content_type_it = part_headers.find("Content-Type");
            if (content_type_it != part_headers.end()) {
                upload.content_type = content_type_it->second;
            }
            
            // Extraire les données du fichier
            size_t data_start = headers_end + 4;
            size_t data_end = request.body.find(boundary, data_start) - 2; // -2 pour ignorer \r\n
            if (data_end > data_start) {
                upload.data = request.body.substr(data_start, data_end - data_start);
                uploads.push_back(upload);
            }
        }
        
        // Passer à la partie suivante
        pos = headers_end + 4;
    }
    
    return uploads;
}

Sauvegarde des fichiers téléversés:

cppCopierbool saveUploadedFile(const FileUpload& upload, const UploadConfig& config) {
    // Vérifier la taille du fichier
    if (upload.data.size() > config.max_file_size) {
        return false;
    }
    
    // Vérifier l'extension si nécessaire
    if (!config.allowed_extensions.empty()) {
        size_t dot_pos = upload.filename.find_last_of(".");
        if (dot_pos == std::string::npos) {
            return false; // Pas d'extension
        }
        
        std::string extension = upload.filename.substr(dot_pos);
        bool extension_allowed = false;
        for (const auto& allowed : config.allowed_extensions) {
            if (extension == allowed) {
                extension_allowed = true;
                break;
            }
        }
        
        if (!extension_allowed) {
            return false;
        }
    }
    
    // Assainir le nom de fichier
    std::string safe_filename = sanitizeFilename(upload.filename);
    
    // Créer le chemin complet
    std::string filepath = config.directory + "/" + safe_filename;
    
    // Sauvegarder le fichier
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(upload.data.c_str(), upload.data.size());
    return file.good();
}

Assainissement des noms de fichiers:

cppCopierstd::string sanitizeFilename(const std::string& filename) {
    std::string result;
    
    // Supprimer les caractères dangereux
    for (char c : filename) {
        // N'autoriser que les caractères alphanumériques, points, tirets et underscores
        if (isalnum(c) || c == '.' || c == '-' || c == '_') {
            result += c;
        }
    }
    
    // Si le nom résultant est vide, utiliser un nom par défaut
    if (result.empty()) {
        result = "upload";
    }
    
    // Ajouter un timestamp pour éviter les écrasements
    time_t now = time(NULL);
    std::string timestamp = std::to_string(now);
    
    // Insérer le timestamp avant l'extension
    size_t dot_pos = result.find_last_of(".");
    if (dot_pos != std::string::npos) {
        result = result.substr(0, dot_pos) + "_" + timestamp + result.substr(dot_pos);
    } else {
        result += "_" + timestamp;
    }
    
    return result;
}

Gestion des requêtes d'upload:

cppCopierHttpResponse handleFileUpload(const HttpRequest& request, const UploadConfig& config) {
    HttpResponse response;
    
    // Vérifier que le répertoire d'upload existe
    if (!directoryExists(config.directory)) {
        return createErrorResponse(500, "Upload directory does not exist");
    }
    
    // Vérifier que la taille totale du body ne dépasse pas la limite
    if (request.body.size() > config.max_file_size) {
        return createErrorResponse(413, "Request Entity Too Large");
    }
    
    // Parser les fichiers téléversés
    std::vector<FileUpload> uploads = parseMultipartFormData(request);
    
    if (uploads.empty()) {
        return createErrorResponse(400, "No files uploaded or invalid format");
    }
    
    // Sauvegarder chaque fichier
    int success_count = 0;
    std::string result = "<html><body><h1>Upload Results</h1><ul>";
    
    for (const auto& upload : uploads) {
        bool success = saveUploadedFile(upload, config);
        if (success) {
            success_count++;
            result += "<li>File '" + htmlEscape(upload.filename) + "' uploaded successfully</li>";
        } else {
            result += "<li>Failed to upload '" + htmlEscape(upload.filename) + "'</li>";
        }
    }
    
    result += "</ul></body></html>";
    
    // Définir le statut de la réponse
    if (success_count == 0) {
        response.setStatus(500);
    } else if (success_count < uploads.size()) {
        response.setStatus(206); // Partial Content
    } else {
        response.setStatus(201); // Created
    }
    
    response.setBody(result, "text/html");
    return response;
}

Handler général pour les requêtes POST:

cppCopierHttpResponse handlePostRequest(const HttpRequest& request, const ServerConfig& server, const LocationConfig& location) {
    // Vérifier si c'est un upload de fichier
    if (!location.upload_directory.empty() && 
        request.headers.count("content-type") && 
        request.headers.at("content-type").find("multipart/form-data") != std::string::npos) {
        
        UploadConfig upload_config;
        upload_config.directory = location.upload_directory;
        upload_config.max_file_size = server.client_max_body_size;
        // Configurer les extensions autorisées si nécessaire
        
        return handleFileUpload(request, upload_config);
    }
    
    // Vérifier s'il s'agit d'un script CGI
    std::string file_path = resolveFilePath(request.uri, server, location);
    if (isCGIScript(file_path, location)) {
        // Traiter comme un script CGI
        CGIConfig cgi_config;
        cgi_config.extension = location.cgi_extension;
        cgi_config.interpreter_path = getCGIInterpreterPath(location.cgi_extension);
        
        return executeCGI(request, file_path, cgi_config, server);
    }
    
    // Traiter comme une requête POST régulière
    // Par exemple, traiter un formulaire application/x-www-form-urlencoded
    if (request.headers.count("content-type") && 
        request.headers.at("content-type").find("application/x-www-form-urlencoded") != std::string::npos) {
        // Traiter les données du formulaire
        return handleFormSubmission(request, server, location);
    }
    
    // Par défaut, renvoyer une erreur Method Not Allowed
    return createErrorResponse(405, "Method Not Allowed");
}

Vérification et création du répertoire d'upload:

cppCopierbool ensureUploadDirectory(const std::string& directory) {
    // Vérifier si le répertoire existe
    struct stat st;
    if (stat(directory.c_str(), &st) == 0) {
        // Le chemin existe, vérifier que c'est un répertoire
        if (S_ISDIR(st.st_mode)) {
            // Vérifier les permissions
            if (access(directory.c_str(), W_OK) == 0) {
                return true; // Le répertoire existe et est accessible en écriture
            } else {
                // Le répertoire existe mais n'est pas accessible en écriture
                return false;
            }
        } else {
            // Le chemin existe mais n'est pas un répertoire
            return false;
        }
    } else {
        // Le répertoire n'existe pas, essayer de le créer
        return (mkdir(directory.c_str(), 0755) == 0);
    }
}

Échappement HTML pour les sorties:

cppCopierstd::string htmlEscape(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += c; break;
        }
    }
    return result;
}
Points de validation

 Parse correctement les requêtes multipart/form-data
 Extrait les données de fichier correctement
 Respecte les limites de taille configurées
 Assainit les noms de fichiers pour la sécurité
 Gère correctement les erreurs d'écriture
 Fournit un retour clair sur le résultat de l'upload

Tests recommandés

Test d'upload simple:

bashCopiercurl -v -F "file=@test.txt" http://localhost:8080/upload

Test avec plusieurs fichiers:

bashCopiercurl -v -F "file1=@test1.txt" -F "file2=@test2.jpg" http://localhost:8080/upload

Test avec un fichier trop grand:

bashCopier# Créer un fichier de 10MB
dd if=/dev/zero of=large_file.bin bs=1M count=10
curl -v -F "file=@large_file.bin" http://localhost:8080/upload

Test avec un type de fichier non autorisé:

bashCopiercurl -v -F "file=@script.php" http://localhost:8080/upload

Test avec des noms de fichiers problématiques:

bashCopier# Créer un fichier avec un nom "dangereux"
echo "test" > "../../../etc/passwd"
curl -v -F "file=@\"../../../etc/passwd\"" http://localhost:8080/upload
Bonnes pratiques

Toujours valider et assainir les noms de fichiers
Limiter la taille des fichiers téléversés
Ne jamais faire confiance aux données fournies par le client
Vérifier les permissions d'écriture dans le répertoire d'upload
Considérer l'utilisation d'un nom de fichier unique (UUID) pour éviter les écrasements
Journaliser les uploads pour des raisons de sécurité
Implémenter une validation des types MIME si nécessaire