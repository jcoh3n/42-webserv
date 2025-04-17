#ifndef FILE_UPLOAD_HANDLER_HPP
#define FILE_UPLOAD_HANDLER_HPP

#include "http/parser/FormData.hpp"
#include "http/upload/UploadConfig.hpp"
#include "http/HttpResponse.hpp"
#include <string>

class FileUploadHandler {
public:
    FileUploadHandler(const UploadConfig& config = UploadConfig())
        : config(config) {}
    
    /**
     * @brief Gère l'upload d'un fichier
     * 
     * @param file Le fichier à uploader
     * @return bool true si l'upload a réussi
     */
    bool handleFileUpload(const UploadedFile& file);
    
    /**
     * @brief Crée une réponse HTML pour l'upload
     * 
     * @param success_count Nombre de fichiers uploadés avec succès
     * @param total_count Nombre total de fichiers
     * @return HttpResponse La réponse HTTP
     */
    static HttpResponse createUploadResponse(int success_count, int total_count);

private:
    /**
     * @brief Vérifie si la taille du fichier est acceptable
     */
    bool checkFileSize(const std::string& content) const;
    
    /**
     * @brief Sauvegarde le fichier sur le disque
     */
    bool saveFile(const std::string& filename, const std::string& content) const;
    
    UploadConfig config;
};

#endif // FILE_UPLOAD_HANDLER_HPP 