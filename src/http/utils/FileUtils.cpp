#include "http/utils/FileUtils.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sstream>
#include <algorithm>
#include <vector>
#include <ctime>
#include <iomanip>
#include <cstring>
#include <cctype>
#include "utils/Common.hpp"

// Vérifier si un fichier existe
bool FileUtils::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// Vérifier si un chemin est un répertoire
bool FileUtils::isDirectory(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        return false;
    }
    return S_ISDIR(buffer.st_mode);
}

// Obtenir la taille d'un fichier
long FileUtils::getFileSize(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        return -1;
    }
    return buffer.st_size;
}

// Vérifier les permissions en lecture
bool FileUtils::hasReadPermission(const std::string& path) {
    return (access(path.c_str(), R_OK) == 0);
}

// Vérifier les permissions en écriture
bool FileUtils::hasWritePermission(const std::string& path) {
    return (access(path.c_str(), W_OK) == 0);
}

// Normaliser un chemin (supprimer les '..' et les '/')
std::string FileUtils::normalizePath(const std::string& base_path, const std::string& uri_path) {
    // Empêcher la traversée de répertoire
    if (uri_path.find("..") != std::string::npos) {
        return "";
    }
    
    // Nettoyer le chemin de base
    std::string path = base_path;
    if (!path.empty() && path[path.size() - 1] == '/') {
        path.erase(path.size() - 1);
    }
    
    // Nettoyer l'URI
    std::string uri = uri_path;
    if (uri.empty() || uri[0] != '/') {
        uri = '/' + uri;
    }
    
    // Vérifier si le chemin de base est absolu
    if (path[0] != '/') {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            path = std::string(cwd) + "/" + path;
        }
    }
    
    // Construire le chemin final
    std::string final_path = path + uri;
    
    // Vérifier que le chemin final est bien dans le répertoire de base
    if (final_path.substr(0, path.length()) != path) {
        return "";
    }
    
    return final_path;
}

// Lister les fichiers d'un répertoire
std::string FileUtils::generateDirectoryListing(const std::string& dir_path, const std::string& request_uri) {
    DIR* dir = opendir(dir_path.c_str());
    if (!dir) {
        return "";
    }
    
    struct dirent* entry;
    std::vector<std::string> files;
    std::vector<std::string> directories;
    
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        
        // Ignorer les fichiers cachés sauf '.' et '..'
        if (name[0] == '.' && name != "." && name != "..") {
            continue;
        }
        
        std::string full_path = dir_path + "/" + name;
        if (isDirectory(full_path)) {
            directories.push_back(name);
        } else {
            files.push_back(name);
        }
    }
    
    closedir(dir);
    
    // Trier les noms
    std::sort(directories.begin(), directories.end());
    std::sort(files.begin(), files.end());
    
    // Générer le HTML avec un design mode sombre inspiré d'Apple
    std::stringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "<head>\n"
         << "    <meta charset=\"UTF-8\">\n"
         << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
         << "    <title>Index of " << request_uri << "</title>\n"
         << "    <style>\n"
         << "        :root {\n"
         << "            --bg-color: #1e1e1e;\n"
         << "            --text-color: #ffffff;\n"
         << "            --secondary-text: #aaaaaa;\n"
         << "            --accent-color: #0a84ff;\n"
         << "            --hover-color: #2c2c2e;\n"
         << "            --border-color: #38383a;\n"
         << "            --header-color: #2c2c2e;\n"
         << "            --dir-color: #0a84ff;\n"
         << "        }\n"
         << "        body {\n"
         << "            font-family: -apple-system, BlinkMacSystemFont, 'SF Pro', 'SF Pro Display', 'Helvetica Neue', Helvetica, Arial, sans-serif;\n"
         << "            margin: 0;\n"
         << "            padding: 0;\n"
         << "            background-color: var(--bg-color);\n"
         << "            color: var(--text-color);\n"
         << "            font-size: 14px;\n"
         << "            line-height: 1.5;\n"
         << "        }\n"
         << "        .container {\n"
         << "            max-width: 960px;\n"
         << "            margin: 0 auto;\n"
         << "            padding: 24px;\n"
         << "        }\n"
         << "        .header {\n"
         << "            border-bottom: 1px solid var(--border-color);\n"
         << "            padding: 0 0 12px 0;\n"
         << "            margin-bottom: 24px;\n"
         << "            display: flex;\n"
         << "            align-items: center;\n"
         << "            justify-content: space-between;\n"
         << "        }\n"
         << "        .header h1 {\n"
         << "            margin: 0;\n"
         << "            font-size: 20px;\n"
         << "            font-weight: 500;\n"
         << "            color: var(--text-color);\n"
         << "        }\n"
         << "        .breadcrumb {\n"
         << "            display: flex;\n"
         << "            flex-wrap: wrap;\n"
         << "            align-items: center;\n"
         << "            margin-bottom: 24px;\n"
         << "            font-size: 13px;\n"
         << "        }\n"
         << "        .breadcrumb a {\n"
         << "            color: var(--accent-color);\n"
         << "            text-decoration: none;\n"
         << "        }\n"
         << "        .breadcrumb a:hover {\n"
         << "            text-decoration: underline;\n"
         << "        }\n"
         << "        .breadcrumb-separator {\n"
         << "            margin: 0 6px;\n"
         << "            color: var(--secondary-text);\n"
         << "        }\n"
         << "        .listing {\n"
         << "            border: 1px solid var(--border-color);\n"
         << "            border-radius: 8px;\n"
         << "            overflow: hidden;\n"
         << "        }\n"
         << "        .listing-header {\n"
         << "            display: grid;\n"
         << "            grid-template-columns: minmax(200px, 1fr) 100px 170px;\n"
         << "            gap: 10px;\n"
         << "            padding: 12px 16px;\n"
         << "            background-color: var(--header-color);\n"
         << "            border-bottom: 1px solid var(--border-color);\n"
         << "            font-weight: 500;\n"
         << "            font-size: 13px;\n"
         << "            color: var(--secondary-text);\n"
         << "        }\n"
         << "        .listing-item {\n"
         << "            display: grid;\n"
         << "            grid-template-columns: minmax(200px, 1fr) 100px 170px;\n"
         << "            gap: 10px;\n"
         << "            padding: 10px 16px;\n"
         << "            border-bottom: 1px solid var(--border-color);\n"
         << "            transition: background-color 0.15s ease;\n"
         << "        }\n"
         << "        .listing-item:last-child {\n"
         << "            border-bottom: none;\n"
         << "        }\n"
         << "        .listing-item:hover {\n"
         << "            background-color: var(--hover-color);\n"
         << "        }\n"
         << "        .listing-item a {\n"
         << "            color: var(--text-color);\n"
         << "            text-decoration: none;\n"
         << "            display: block;\n"
         << "            overflow: hidden;\n"
         << "            text-overflow: ellipsis;\n"
         << "            white-space: nowrap;\n"
         << "        }\n"
         << "        .listing-item.directory a {\n"
         << "            color: var(--dir-color);\n"
         << "            font-weight: 500;\n"
         << "        }\n"
         << "        .listing-item.text-file a {\n"
         << "            color: #34c759;\n"
         << "        }\n"
         << "        .listing-item.image-file a {\n"
         << "            color: #ff9f0a;\n"
         << "        }\n"
         << "        .listing-item.code-file a {\n"
         << "            color: #5e5ce6;\n"
         << "        }\n"
         << "        .listing-item a:hover {\n"
         << "            text-decoration: underline;\n"
         << "        }\n"
         << "        .parent-dir {\n"
         << "            background-color: var(--header-color);\n"
         << "        }\n"
         << "        .file-size, .file-date {\n"
         << "            color: var(--secondary-text);\n"
         << "            font-size: 13px;\n"
         << "        }\n"
         << "        .server-info {\n"
         << "            margin-top: 16px;\n"
         << "            text-align: center;\n"
         << "            font-size: 12px;\n"
         << "            color: var(--secondary-text);\n"
         << "        }\n"
         << "        @media (max-width: 768px) {\n"
         << "            .listing-header, .listing-item {\n"
         << "                grid-template-columns: 1fr 100px;\n"
         << "            }\n"
         << "            .file-date {\n"
         << "                display: none;\n"
         << "            }\n"
         << "        }\n"
         << "        @media (max-width: 480px) {\n"
         << "            .listing-header, .listing-item {\n"
         << "                grid-template-columns: 1fr;\n"
         << "            }\n"
         << "            .file-size {\n"
         << "                display: none;\n"
         << "            }\n"
         << "        }\n"
         << "        @media (prefers-color-scheme: light) {\n"
         << "            :root {\n"
         << "                --bg-color: #ffffff;\n"
         << "                --text-color: #1d1d1f;\n"
         << "                --secondary-text: #86868b;\n"
         << "                --accent-color: #0071e3;\n"
         << "                --hover-color: #f5f5f7;\n"
         << "                --border-color: #d2d2d7;\n"
         << "                --header-color: #f5f5f7;\n"
         << "                --dir-color: #0071e3;\n"
         << "            }\n"
         << "        }\n"
         << "    </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "    <div class=\"container\">\n"
         << "        <div class=\"header\">\n"
         << "            <h1>Index of " << request_uri << "</h1>\n"
         << "        </div>\n"
         << "        <div class=\"breadcrumb\">\n";
    
    // Générer le fil d'Ariane (breadcrumb)
    std::string path = "/";
    html << "            <a href=\"/\">Home</a>";
    
    if (request_uri != "/") {
        std::string uri = request_uri;
        if (uri[uri.size() - 1] == '/') {
            uri = uri.substr(0, uri.size() - 1);
        }
        
        std::vector<std::string> components;
        size_t start = 0;
        size_t end = 0;
        
        while ((end = uri.find('/', start)) != std::string::npos) {
            if (end != start) {
                components.push_back(uri.substr(start, end - start));
            }
            start = end + 1;
        }
        
        if (start < uri.size()) {
            components.push_back(uri.substr(start));
        }
        
        std::string current_path = "";
        for (size_t i = 0; i < components.size(); ++i) {
            current_path += "/" + components[i];
            html << "            <span class=\"breadcrumb-separator\">/</span>\n";
            
            if (i == components.size() - 1) {
                html << "            <span>" << components[i] << "</span>\n";
            } else {
                html << "            <a href=\"" << current_path << "/\">" << components[i] << "</a>\n";
            }
        }
    }
    
    html << "        </div>\n"
         << "        <div class=\"listing\">\n"
         << "            <div class=\"listing-header\">\n"
         << "                <div>Name</div>\n"
         << "                <div>Size</div>\n"
         << "                <div>Last Modified</div>\n"
         << "            </div>\n";
    
    // Ajouter le lien vers le répertoire parent
    if (request_uri != "/") {
        std::string parent_uri = request_uri;
        if (!parent_uri.empty() && parent_uri[parent_uri.size() - 1] == '/') {
            parent_uri.erase(parent_uri.size() - 1);
        }
        
        size_t last_slash = parent_uri.find_last_of('/');
        if (last_slash != std::string::npos) {
            parent_uri = parent_uri.substr(0, last_slash + 1);
        }
        
        html << "            <div class=\"listing-item parent-dir directory\">\n"
             << "                <div>\n"
             << "                    <a href=\"" << parent_uri << "\">Parent Directory</a>\n"
             << "                </div>\n"
             << "                <div class=\"file-size\">-</div>\n"
             << "                <div class=\"file-date\">-</div>\n"
             << "            </div>\n";
    }
    
    // Ajouter les répertoires
    for (size_t i = 0; i < directories.size(); ++i) {
        const std::string& dir_name = directories[i];
        if (dir_name == "." || dir_name == "..") {
            continue; // On ignore . et ..
        }
        
        std::string uri = request_uri;
        if (!uri.empty() && uri[uri.size() - 1] != '/') {
            uri += '/';
        }
        
        uri += dir_name + '/';
        
        std::string full_path = dir_path + "/" + dir_name;
        struct stat st;
        stat(full_path.c_str(), &st);
        
        // Formater la date de dernière modification
        char time_buf[64];
        struct tm* tm_info = localtime(&st.st_mtime);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
        
        html << "            <div class=\"listing-item directory\">\n"
             << "                <div>\n"
             << "                    <a href=\"" << uri << "\">" << dir_name << "</a>\n"
             << "                </div>\n"
             << "                <div class=\"file-size\">-</div>\n"
             << "                <div class=\"file-date\">" << time_buf << "</div>\n"
             << "            </div>\n";
    }
    
    // Ajouter les fichiers
    for (size_t i = 0; i < files.size(); ++i) {
        const std::string& file_name = files[i];
        std::string uri = request_uri;
        if (!uri.empty() && uri[uri.size() - 1] != '/') {
            uri += '/';
        }
        
        uri += file_name;
        
        std::string full_path = dir_path + "/" + file_name;
        struct stat st;
        stat(full_path.c_str(), &st);
        
        // Formater la taille du fichier
        std::stringstream size_stream;
        if (st.st_size < 1024) {
            size_stream << st.st_size << " B";
        } else if (st.st_size < 1024 * 1024) {
            size_stream << std::fixed << std::setprecision(1) << (st.st_size / 1024.0) << " KB";
        } else if (st.st_size < 1024 * 1024 * 1024) {
            size_stream << std::fixed << std::setprecision(1) << (st.st_size / (1024.0 * 1024.0)) << " MB";
        } else {
            size_stream << std::fixed << std::setprecision(1) << (st.st_size / (1024.0 * 1024.0 * 1024.0)) << " GB";
        }
        
        // Formater la date de dernière modification
        char time_buf[64];
        struct tm* tm_info = localtime(&st.st_mtime);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
        
        // Détecter le type de fichier pour appliquer un style
        std::string file_class = getFileClass(file_name);
        
        html << "            <div class=\"" << file_class << "\">\n"
             << "                <div>\n"
             << "                    <a href=\"" << uri << "\">" << file_name << "</a>\n"
             << "                </div>\n"
             << "                <div class=\"file-size\">" << size_stream.str() << "</div>\n"
             << "                <div class=\"file-date\">" << time_buf << "</div>\n"
             << "            </div>\n";
    }
    
    html << "        </div>\n"
         << "        <div class=\"server-info\">\n"
         << "            <p>webserv Server at " << request_uri << "</p>\n"
         << "        </div>\n"
         << "    </div>\n"
         << "</body>\n"
         << "</html>";
    
    return html.str();
}

// Fonction pour déterminer la classe CSS d'un fichier basé sur son extension
std::string FileUtils::getFileClass(const std::string& file_name) {
    std::string file_class = "listing-item";
    std::string extension = "";
    
    size_t dot_pos = file_name.find_last_of('.');
    if (dot_pos != std::string::npos) {
        extension = file_name.substr(dot_pos);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        // Fichiers texte
        if (extension == ".txt" || extension == ".md" || extension == ".csv" || 
            extension == ".log" || extension == ".conf") {
            file_class += " text-file";
        } 
        // Fichiers code
        else if (extension == ".html" || extension == ".htm" || extension == ".css" || 
                 extension == ".js" || extension == ".php" || extension == ".py" || 
                 extension == ".json" || extension == ".xml") {
            file_class += " code-file";
        } 
        // Images
        else if (extension == ".jpg" || extension == ".jpeg" || extension == ".png" || 
                 extension == ".gif" || extension == ".svg" || extension == ".webp") {
            file_class += " image-file";
        }
    }
    
    return file_class;
}

// Assainir un nom de fichier pour l'upload
std::string FileUtils::sanitizeFilename(const std::string& filename) {
    std::string result;
    
    // Supprimer les caractères dangereux
    for (size_t i = 0; i < filename.length(); ++i) {
        char c = filename[i];
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
    
    // Convertir le timestamp en string (C++98 compatible)
    std::stringstream ss;
    ss << "_" << now;
    std::string timestamp = ss.str();
    
    // Insérer le timestamp avant l'extension
    size_t dot_pos = result.find_last_of(".");
    if (dot_pos != std::string::npos) {
        result = result.substr(0, dot_pos) + timestamp + result.substr(dot_pos);
    } else {
        result += timestamp;
    }
    
    return result;
}

// Echapper les caractères HTML pour l'affichage sécurisé
std::string FileUtils::htmlEscape(const std::string& text) {
    std::string result;
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += c;
        }
    }
    return result;
}

// S'assurer qu'un répertoire existe, le créer si nécessaire
bool FileUtils::ensureDirectoryExists(const std::string& path) {
    // Si le répertoire existe déjà
    if (isDirectory(path)) {
        return true;
    }
    
    // Si le chemin existe mais n'est pas un répertoire
    if (fileExists(path)) {
        return false;
    }
    
    // Créer le répertoire avec les permissions 0755
    return (mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0);
} 