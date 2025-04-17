#include "http/parser/FormParser.hpp"
#include "http/utils/HttpUtils.hpp"
#include "utils/Common.hpp"
#include <sstream>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <cstring>

void FormParser::parseUrlEncoded(const std::string& body, FormData& form_data) {
    std::istringstream iss(body);
    std::string pair;
    
    // Séparer par les '&'
    while (std::getline(iss, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos == std::string::npos)
            continue;
        
        std::string key = pair.substr(0, eq_pos);
        std::string value = pair.substr(eq_pos + 1);
        
        // Décoder les caractères URL-encoded
        form_data.addFormValue(HttpUtils::urlDecode(key), HttpUtils::urlDecode(value));
    }
}

void FormParser::parseMultipart(const std::string& body, const std::string& boundary, FormData& form_data) {
    // Extraire la boundary du Content-Type si elle contient des guillemets
    std::string clean_boundary = boundary;
    size_t quote_start = boundary.find("\"");
    if (quote_start != std::string::npos) {
        size_t quote_end = boundary.find("\"", quote_start + 1);
        if (quote_end != std::string::npos) {
            clean_boundary = boundary.substr(quote_start + 1, quote_end - quote_start - 1);
        }
    }
    
    std::string full_boundary = "--" + clean_boundary;
    std::string end_boundary = "--" + clean_boundary + "--";
    
    LOG_INFO("Using boundary: '" << full_boundary << "'");
    LOG_INFO("Body size: " << body.size() << " bytes");
    LOG_INFO("Body content: '" << body.substr(0, 100) << "...'");
    
    // Découper le corps en parties séparées par la boundary
    size_t pos = 0;
    size_t boundary_pos;
    
    // Trouver la première boundary
    boundary_pos = body.find(full_boundary);
    if (boundary_pos == std::string::npos) {
        LOG_ERROR("No boundary found in body");
        return;
    }
    
    pos = boundary_pos + full_boundary.length();
    
    while (pos < body.length()) {
        // Vérifier si nous sommes à la fin
        if (body.substr(boundary_pos, end_boundary.length()) == end_boundary) {
            break;
        }
        
        // Trouver la prochaine boundary
        boundary_pos = body.find(full_boundary, pos);
        if (boundary_pos == std::string::npos) {
            // Si pas de boundary suivante, utiliser la fin du corps
            boundary_pos = body.length();
        }
        
        // Extraire la partie entre les boundaries
        std::string part = body.substr(pos, boundary_pos - pos);
        
        // Supprimer les caractères CR+LF au début
        while (part.length() >= 2 && part.substr(0, 2) == "\r\n") {
            part = part.substr(2);
        }
        
        // Supprimer les caractères CR+LF à la fin
        while (part.length() >= 2 && part.substr(part.length() - 2) == "\r\n") {
            part = part.substr(0, part.length() - 2);
        }
        
        // Parser cette partie si elle n'est pas vide
        if (!part.empty()) {
            LOG_INFO("Parsing part: '" << part << "'");
            parseMultipartPart(part, form_data);
        }
        
        // Avancer après la boundary courante
        pos = boundary_pos + full_boundary.length();
    }
    
    LOG_INFO("Multipart form parsed: " << form_data.getFormValues().size() << " fields, " 
             << form_data.getUploadedFiles().size() << " files");
}

void FormParser::parseMultipartPart(const std::string& part, FormData& form_data) {
    // Trouver la séparation entre headers et body
    size_t header_end = part.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        LOG_ERROR("Invalid multipart part: no header separation");
        return;
    }
    
    // Extraire les headers et le contenu
    std::string headers = part.substr(0, header_end);
    std::string content = part.substr(header_end + 4); // +4 pour sauter "\r\n\r\n"
    
    // Supprimer le dernier \r\n du contenu s'il existe
    if (content.length() >= 2 && content.substr(content.length() - 2) == "\r\n") {
        content = content.substr(0, content.length() - 2);
    }
    
    // Analyser les headers
    std::string name, filename, content_type;
    extractPartHeaders(headers, name, filename, content_type);
    
    LOG_INFO("Parsed part - name: '" << name << "', filename: '" << filename << "', content: '" << content << "'");
    
    // Si un nom de fichier est présent, c'est un upload de fichier
    if (!filename.empty()) {
        UploadedFile file;
        file.name = name;
        file.filename = filename;
        file.content_type = content_type;
        file.data = content;
        
        form_data.addUploadedFile(name, file);
        
        LOG_INFO("Uploaded file: " << filename << " (" << content.size() << " bytes, "
                 << content_type << ")");
        
        // Sauvegarder le fichier dans le dossier uploads
        std::string upload_dir = "./www/uploads/";
        
        // Créer le répertoire si nécessaire
        struct stat info;
        if (stat(upload_dir.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR)) {
            LOG_WARNING("Upload directory does not exist, creating: " << upload_dir);
            if (mkdir(upload_dir.c_str(), 0755) != 0) {
                LOG_ERROR("Failed to create upload directory: " << upload_dir);
                return;
            }
        }
        
        // Enregistrer le fichier
        std::string file_path = upload_dir + filename;
        std::ofstream file_out(file_path.c_str(), std::ios::binary);
        if (!file_out) {
            LOG_ERROR("Failed to save uploaded file: " << file_path);
            return;
        }
        
        file_out.write(content.data(), content.size());
        file_out.close();
        
        LOG_SUCCESS("File saved to: " << file_path);
    }
    // Sinon, c'est une valeur de formulaire standard
    else {
        form_data.addFormValue(name, content);
        LOG_INFO("Form field: " << name << " = " << content);
    }
}

void FormParser::extractPartHeaders(const std::string& part_headers, 
                                   std::string& name, 
                                   std::string& filename, 
                                   std::string& content_type) {
    // Initialiser les variables de sortie
    name.clear();
    filename.clear();
    content_type = "application/octet-stream";  // Valeur par défaut
    
    // Diviser les headers par lignes
    std::istringstream iss(part_headers);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Supprimer les retours chariot à la fin si présents
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        // Ignorer les lignes vides
        if (line.empty()) {
            continue;
        }
        
        // Trouver le séparateur ': ' pour les headers
        size_t colon = line.find(": ");
        if (colon != std::string::npos) {
            std::string header_name = line.substr(0, colon);
            std::string header_value = line.substr(colon + 2);
            
            // Content-Disposition
            if (header_name == "Content-Disposition") {
                // Trouver name="xxx"
                size_t name_pos = header_value.find("name=\"");
                if (name_pos != std::string::npos) {
                    size_t name_start = name_pos + 6;  // Longueur de 'name="'
                    size_t name_end = header_value.find("\"", name_start);
                    if (name_end != std::string::npos) {
                        name = header_value.substr(name_start, name_end - name_start);
                    }
                }
                
                // Trouver filename="xxx"
                size_t filename_pos = header_value.find("filename=\"");
                if (filename_pos != std::string::npos) {
                    size_t filename_start = filename_pos + 10;  // Longueur de 'filename="'
                    size_t filename_end = header_value.find("\"", filename_start);
                    if (filename_end != std::string::npos) {
                        filename = header_value.substr(filename_start, filename_end - filename_start);
                    }
                }
            }
            // Content-Type
            else if (header_name == "Content-Type") {
                content_type = header_value;
            }
        }
    }
    
    // S'assurer que le nom est non vide
    if (name.empty()) {
        LOG_ERROR("Missing name in Content-Disposition header");
    }
} 