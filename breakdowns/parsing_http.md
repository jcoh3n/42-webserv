## ÉTAPE 3 - PARSING DES REQUÊTES HTTP

### Objectif
Implémenter un parseur capable d'analyser les requêtes HTTP entrantes et d'en extraire les éléments essentiels, y compris le support pour les requêtes multipart/form-data.

### Structure d'une requête HTTP typique
GET /index.html HTTP/1.1\r\n
Host: localhost:8080\r\n
User-Agent: curl/7.68.0\r\n
Accept: /\r\n
\r\n
[Body pour les requêtes POST]
Copier
### Détails techniques

1. **Structure de données pour stocker la requête**:
```cpp
class HttpRequest {
public:
    std::string method;      // GET, POST, DELETE
    std::string uri;         // /index.html
    std::string version;     // HTTP/1.1
    std::map<std::string, std::string> headers;
    std::string body;
    std::string query_string; // Paramètres GET après le '?'
    
    // Pour les uploads multipart
    struct UploadedFile {
        std::string name;
        std::string filename;
        std::string content_type;
        std::string data;
    };
    std::map<std::string, UploadedFile> uploaded_files;
    std::map<std::string, std::string> form_values;
    
    void clear() {
        method.clear();
        uri.clear();
        version.clear();
        headers.clear();
        body.clear();
        query_string.clear();
        uploaded_files.clear();
        form_values.clear();
    }
};

Algorithme de parsing:

cppCopierbool parseRequest(const std::string& request, HttpRequest& req) {
    req.clear();
    
    // Trouver la fin de la ligne de commande
    size_t end_of_line = request.find("\r\n");
    if (end_of_line == std::string::npos) return false;
    
    // Parser la ligne de commande
    std::string request_line = request.substr(0, end_of_line);
    std::istringstream iss(request_line);
    if (!(iss >> req.method >> req.uri >> req.version)) {
        return false;
    }
    
    // Normaliser les méthodes HTTP
    std::transform(req.method.begin(), req.method.end(), req.method.begin(), ::toupper);
    
    // Extraire la query string
    size_t query_pos = req.uri.find('?');
    if (query_pos != std::string::npos) {
        req.query_string = req.uri.substr(query_pos + 1);
        req.uri = req.uri.substr(0, query_pos);
    }
    
    // Protection contre la traversée de répertoire
    if (req.uri.find("..") != std::string::npos) {
        return false;
    }
    
    // Parser les headers
    size_t pos = end_of_line + 2; // Sauter \r\n
    while (true) {
        size_t header_end = request.find("\r\n", pos);
        if (header_end == std::string::npos) return false;
        
        // Vérifier si c'est la fin des headers
        if (pos == header_end) {
            pos = header_end + 2; // Sauter la ligne vide
            break;
        }
        
        std::string header_line = request.substr(pos, header_end - pos);
        size_t colon = header_line.find(":");
        if (colon != std::string::npos) {
            std::string name = header_line.substr(0, colon);
            std::string value = header_line.substr(colon + 1);
            
            // Normaliser le nom du header (lowercase)
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            
            // Supprimer les espaces en début/fin
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            req.headers[name] = value;
        }
        
        pos = header_end + 2; // Sauter \r\n
    }
    
    // Extraire le body
    if (pos < request.size()) {
        req.body = request.substr(pos);
        
        // Si c'est un POST multipart/form-data, parser les données
        auto it = req.headers.find("content-type");
        if (req.method == "POST" && it != req.headers.end() && 
            it->second.find("multipart/form-data") != std::string::npos) {
            parseMultipartFormData(req);
        }
        // Si c'est un POST application/x-www-form-urlencoded
        else if (req.method == "POST" && it != req.headers.end() && 
                 it->second.find("application/x-www-form-urlencoded") != std::string::npos) {
            parseUrlEncodedForm(req);
        }
    }
    
    return true;
}

Parsing des données multipart/form-data:

cppCopiervoid parseMultipartFormData(HttpRequest& req) {
    // Extraire la boundary du Content-Type
    std::string content_type = req.headers["content-type"];
    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string::npos) return;
    
    std::string boundary = "--" + content_type.substr(boundary_pos + 9);
    
    // Diviser le body en parties avec la boundary
    size_t pos = 0;
    while ((pos = req.body.find(boundary, pos)) != std::string::npos) {
        pos += boundary.length();
        
        // Vérifier si c'est la fin
        if (req.body.substr(pos, 2) == "--") {
            break;
        }
        
        // Sauter les \r\n
        if (req.body[pos] == '\r') pos += 2;
        
        // Trouver la fin des headers de cette partie
        size_t headers_end = req.body.find("\r\n\r\n", pos);
        if (headers_end == std::string::npos) break;
        
        // Extraire les headers de cette partie
        std::string part_headers = req.body.substr(pos, headers_end - pos);
        
        // Analyser le Content-Disposition pour obtenir name et filename
        std::string name, filename, content_type;
        
        size_t cd_pos = part_headers.find("Content-Disposition:");
        if (cd_pos != std::string::npos) {
            size_t name_pos = part_headers.find("name=\"", cd_pos);
            if (name_pos != std::string::npos) {
                name_pos += 6; // Longueur de 'name="'
                size_t name_end = part_headers.find("\"", name_pos);
                if (name_end != std::string::npos) {
                    name = part_headers.substr(name_pos, name_end - name_pos);
                }
            }
            
            size_t filename_pos = part_headers.find("filename=\"", cd_pos);
            if (filename_pos != std::string::npos) {
                filename_pos += 10; // Longueur de 'filename="'
                size_t filename_end = part_headers.find("\"", filename_pos);
                if (filename_end != std::string::npos) {
                    filename = part_headers.substr(filename_pos, filename_end - filename_pos);
                }
            }
        }
        
        // Analyser le Content-Type de cette partie
        size_t ct_pos = part_headers.find("Content-Type:");
        if (ct_pos != std::string::npos) {
            size_t ct_end = part_headers.find("\r\n", ct_pos);
            if (ct_end != std::string::npos) {
                ct_pos += 14; // Longueur de 'Content-Type: '
                content_type = part_headers.substr(ct_pos, ct_end - ct_pos);
            }
        }
        
        // Extraire les données de cette partie
        pos = headers_end + 4; // Sauter "\r\n\r\n"
        size_t part_end = req.body.find(boundary, pos);
        if (part_end == std::string::npos) break;
        
        // Les données se terminent par \r\n avant la boundary
        size_t data_end = part_end - 2;
        std::string data = req.body.substr(pos, data_end - pos);
        
        // Stocker les données selon qu'il s'agit d'un fichier ou d'une valeur de formulaire
        if (!filename.empty()) {
            HttpRequest::UploadedFile file;
            file.name = name;
            file.filename = filename;
            file.content_type = content_type;
            file.data = data;
            req.uploaded_files[name] = file;
        } else if (!name.empty()) {
            req.form_values[name] = data;
        }
        
        // Continuer avec la partie suivante
        pos = part_end;
    }
}

Parsing des données application/x-www-form-urlencoded:

cppCopiervoid parseUrlEncodedForm(HttpRequest& req) {
    std::istringstream stream(req.body);
    std::string pair;
    
    while (std::getline(stream, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos == std::string::npos) continue;
        
        std::string key = pair.substr(0, eq_pos);
        std::string value = pair.substr(eq_pos + 1);
        
        // Décoder URL
        req.form_values[urlDecode(key)] = urlDecode(value);
    }
}

Décodage URL:

cppCopierstd::string urlDecode(const std::string& encoded) {
    std::string result;
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            int value;
            std::istringstream is(encoded.substr(i + 1, 2));
            if (is >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += encoded[i];
            }
        } else if (encoded[i] == '+') {
            result += ' ';
        } else {
            result += encoded[i];
        }
    }
    return result;
}

Validation de la requête:

cppCopierbool validateRequest(const HttpRequest& req) {
    // Vérifier les champs requis
    if (req.method.empty() || req.uri.empty() || req.version.empty()) {
        return false;
    }
    
    // Vérifier les méthodes supportées
    if (req.method != "GET" && req.method != "POST" && req.method != "DELETE") {
        return false;
    }
    
    // Vérifier la version HTTP
    if (req.version != "HTTP/1.1" && req.version != "HTTP/1.0") {
        return false;
    }
    
    // Vérifier l'en-tête Host pour HTTP/1.1
    if (req.version == "HTTP/1.1" && req.headers.find("host") == req.headers.end()) {
        return false;
    }
    
    // Vérifier la taille maximale du body (si configurée)
    auto it = req.headers.find("content-length");
    if (it != req.headers.end()) {
        size_t content_length = std::stoul(it->second);
        if (content_length > MAX_BODY_SIZE) {
            return false;
        }
    }
    
    return true;
}
Points de validation

 Parse correctement une requête GET simple
 Extrait correctement tous les headers
 Gère correctement le body des requêtes POST
 Parse correctement les données multipart/form-data
 Parse correctement les données application/x-www-form-urlencoded
 Détecte les requêtes malformées
 Valide les méthodes HTTP supportées
 Protège contre les attaques de traversée de répertoire
 Normalise les noms de headers

Tests recommandés

Requête GET basique:

bashCopierprintf "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080

Requête avec query string:

bashCopierprintf "GET /search?q=webserv&page=1 HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080

Requête POST avec body:

bashCopierprintf "POST /login HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 27\r\n\r\nusername=admin&password=1234" | nc localhost 8080

Requête avec upload de fichier:

bashCopierprintf -- "--boundary\r\nContent-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\nContent-Type: text/plain\r\n\r\nHello, World!\r\n--boundary--\r\n" | curl -X POST -H "Content-Type: multipart/form-data; boundary=boundary" --data-binary @- http://localhost:8080/upload

Tentative de traversée de répertoire:

bashCopierprintf "GET /../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080

Requête malformée:

bashCopierprintf "INVALID / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080
Bonnes pratiques

Limiter la taille maximale des headers et du body
Normaliser les noms de headers (conversion en minuscules)
Valider rigoureusement les URI pour éviter les attaques
Gérer les encodages de caractères (UTF-8)
Implémenter un mécanisme de timeout pour éviter les attaques de déni de service
Rejeter les requêtes avec des headers trop longs