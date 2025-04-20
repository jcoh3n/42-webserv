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
    
    // Vérifier les permissions en écriture
    static bool hasWritePermission(const std::string& path);
    
    // Normaliser un chemin (supprimer les '..' et les '/')
    static std::string normalizePath(const std::string& base_path, const std::string& uri_path);
    
    // Lister les fichiers d'un répertoire (pour les index automatiques)
    static std::string generateDirectoryListing(const std::string& dir_path, const std::string& request_uri);
    
    // Assainir un nom de fichier pour l'upload
    static std::string sanitizeFilename(const std::string& filename);
    
    // Echapper les caractères HTML pour l'affichage sécurisé
    static std::string htmlEscape(const std::string& text);
    
    // S'assurer qu'un répertoire existe, le créer si nécessaire
    static bool ensureDirectoryExists(const std::string& path);
    
    // Fonction pour déterminer la classe CSS d'un fichier basé sur son extension
    static std::string getFileClass(const std::string& file_name);
};

#endif // FILE_UTILS_HPP 