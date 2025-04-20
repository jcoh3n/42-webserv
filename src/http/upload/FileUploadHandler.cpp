#include "http/upload/FileUploadHandler.hpp"
#include "utils/Common.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>

bool FileUploadHandler::handleFileUpload(const UploadedFile& file) {
    if (!config.ensureUploadDirectory()) {
        LOG_ERROR("Failed to ensure upload directory exists");
        return false;
    }
    
    // Vérifier la taille du fichier
    if (file.data.size() > config.getMaxFileSize()) {
        LOG_ERROR("File too large: " << file.filename << " (" << file.data.size() / 1024 << " KB)");
        return false;
    }
    
    // Sécuriser le nom du fichier et le sauvegarder
    std::string safe_name = sanitizeFilename(file.filename);
    if (saveFile(safe_name, file.data)) {
        LOG_UPLOAD("File uploaded: " << safe_name << " (" << file.data.size() / 1024 << " KB)");
        return true;
    }
    
    return false;
}

HttpResponse FileUploadHandler::createUploadResponse(int success_count, int total_count) {
    HttpResponse response;
    std::ostringstream html;
    
    html << "<!DOCTYPE html><html><head><title>Upload Results</title>"
         << "<style>body{font-family:sans-serif;max-width:800px;margin:0 auto;padding:20px}</style></head>"
         << "<body><h1>Upload Results</h1>";
    
    if (success_count == total_count) {
        response.setStatus(201);
        html << "<p style='color:green'>All files uploaded successfully!</p>";
    } else if (success_count > 0) {
        response.setStatus(206);
        html << "<p style='color:orange'>Some files uploaded successfully.</p>";
    } else {
        response.setStatus(500);
        html << "<p style='color:red'>No files were uploaded successfully.</p>";
    }
    
    html << "<p>Successfully uploaded: <strong>" << success_count << "/" << total_count 
         << "</strong></p><p><a href='/pages/features/upload.html'>Back to upload page</a></p></body></html>";
    
    response.setBody(html.str(), "text/html");
    return response;
}

std::string FileUploadHandler::sanitizeFilename(const std::string& filename) const {
    std::string clean_name = filename;
    
    // Supprimer les chemins absolus ou relatifs
    size_t last_slash = clean_name.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        clean_name = clean_name.substr(last_slash + 1);
    }
    
    // Remplacer les caractères problématiques
    std::string invalid_chars = "<>:\"|?*";
    for (size_t i = 0; i < clean_name.length(); ++i) {
        if (invalid_chars.find(clean_name[i]) != std::string::npos) {
            clean_name[i] = '_';
        }
    }
    
    return clean_name.empty() ? "unnamed_file" : clean_name;
}

bool FileUploadHandler::saveFile(const std::string& filename, const std::string& content) const {
    std::string file_path = config.getUploadDirectory() + filename;
    std::string final_filename = filename;
    
    // Gérer les doublons simplement
    if (access(file_path.c_str(), F_OK) != -1) {
        // Extraire le nom et l'extension
        size_t dot_pos = filename.find_last_of('.');
        std::string name = (dot_pos != std::string::npos) ? filename.substr(0, dot_pos) : filename;
        std::string ext = (dot_pos != std::string::npos) ? filename.substr(dot_pos) : "";
        
        // Ajouter un timestamp
        std::ostringstream oss;
        oss << name << "_" << time(NULL) << ext;
        final_filename = oss.str();
        file_path = config.getUploadDirectory() + final_filename;
    }
    
    // Écrire le fichier
    std::ofstream file_out(file_path.c_str(), std::ios::binary);
    if (!file_out || file_out.write(content.data(), content.size()).fail()) {
        LOG_ERROR("Failed to write file: " << file_path);
        return false;
    }
    
    file_out.close();
    // Pas besoin de log ici, on affichera juste le message d'upload
    return true;
} 