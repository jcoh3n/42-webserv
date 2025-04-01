#ifndef FILE_UTILS_HPP
#define FILE_UTILS_HPP

#include <string>

/**
 * @brief Classe utilitaire pour les opérations sur les fichiers
 */
class FileUtils {
public:
    // Vérifier si un fichier existe
    static bool fileExists(const std::string& path);
    
    // Vérifier si un chemin est un répertoire
    static bool isDirectory(const std::string& path);
    
    // Obtenir la taille d'un fichier
    static long getFileSize(const std::string& path);
    
    // Vérifier les permissions en lecture
    static bool hasReadPermission(const std::string& path);
    
    // Normaliser un chemin (supprimer les '..' et les '/')
    static std::string normalizePath(const std::string& base_path, const std::string& uri_path);
    
    // Lister les fichiers d'un répertoire (pour les index automatiques)
    static std::string generateDirectoryListing(const std::string& dir_path, const std::string& request_uri);
};

#endif // FILE_UTILS_HPP 