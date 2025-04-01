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
    
    // Générer le HTML
    std::stringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "<head>\n"
         << "    <title>Index of " << request_uri << "</title>\n"
         << "    <style>\n"
         << "        body { font-family: Arial, sans-serif; margin: 20px; }\n"
         << "        h1 { border-bottom: 1px solid #ddd; padding-bottom: 10px; }\n"
         << "        table { border-collapse: collapse; width: 100%; }\n"
         << "        th, td { text-align: left; padding: 8px; }\n"
         << "        tr:nth-child(even) { background-color: #f2f2f2; }\n"
         << "        th { background-color: #4CAF50; color: white; }\n"
         << "        a { text-decoration: none; color: #2196F3; }\n"
         << "        a:hover { text-decoration: underline; }\n"
         << "    </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "    <h1>Index of " << request_uri << "</h1>\n"
         << "    <table>\n"
         << "        <tr>\n"
         << "            <th>Name</th>\n"
         << "            <th>Type</th>\n"
         << "            <th>Size</th>\n"
         << "            <th>Last Modified</th>\n"
         << "        </tr>\n";
    
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
        
        html << "        <tr>\n"
             << "            <td><a href=\"" << parent_uri << "\">Parent Directory</a></td>\n"
             << "            <td>Directory</td>\n"
             << "            <td>-</td>\n"
             << "            <td>-</td>\n"
             << "        </tr>\n";
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
        
        html << "        <tr>\n"
             << "            <td><a href=\"" << uri << "\">" << dir_name << "/</a></td>\n"
             << "            <td>Directory</td>\n"
             << "            <td>-</td>\n"
             << "            <td>" << time_buf << "</td>\n"
             << "        </tr>\n";
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
        
        html << "        <tr>\n"
             << "            <td><a href=\"" << uri << "\">" << file_name << "</a></td>\n"
             << "            <td>File</td>\n"
             << "            <td>" << size_stream.str() << "</td>\n"
             << "            <td>" << time_buf << "</td>\n"
             << "        </tr>\n";
    }
    
    html << "    </table>\n"
         << "    <hr>\n"
         << "    <p>webserv Server at " << request_uri << "</p>\n"
         << "</body>\n"
         << "</html>";
    
    return html.str();
} 