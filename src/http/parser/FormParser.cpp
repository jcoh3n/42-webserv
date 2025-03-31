#include "http/parser/FormParser.hpp"
#include "http/utils/HttpUtils.hpp"
#include <sstream>

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
    std::string full_boundary = "--" + boundary;
    
    // Diviser le body en parties avec la boundary
    size_t pos = 0;
    while ((pos = body.find(full_boundary, pos)) != std::string::npos) {
        pos += full_boundary.length();
        
        // Vérifier si c'est la fin
        if (pos + 2 <= body.length() && body.substr(pos, 2) == "--") {
            break;
        }
        
        // Sauter les \r\n
        if (pos < body.length() && body[pos] == '\r')
            pos += 2;
        
        // Trouver la fin des headers de cette partie
        size_t headers_end = body.find("\r\n\r\n", pos);
        if (headers_end == std::string::npos)
            break;
        
        // Extraire les headers de cette partie
        std::string part_headers = body.substr(pos, headers_end - pos);
        
        // Extraire les données de cette partie
        pos = headers_end + 4; // Sauter "\r\n\r\n"
        size_t part_end = body.find(full_boundary, pos);
        if (part_end == std::string::npos)
            break;
        
        // Les données se terminent par \r\n avant la boundary
        size_t data_end = part_end - 2;
        std::string data = body.substr(pos, data_end - pos);
        
        // Extraire les informations des headers
        std::string name, filename, content_type;
        extractPartHeaders(part_headers, name, filename, content_type);
        
        // Stocker selon qu'il s'agit d'un fichier ou d'une valeur
        if (!filename.empty() && !name.empty()) {
            UploadedFile file;
            file.name = name;
            file.filename = filename;
            file.content_type = content_type;
            file.data = data;
            form_data.addUploadedFile(name, file);
        } 
        else if (!name.empty()) {
            form_data.addFormValue(name, data);
        }
        
        // Continuer avec la partie suivante
        pos = part_end;
    }
}

void FormParser::extractPartHeaders(const std::string& part_headers, 
                                   std::string& name, 
                                   std::string& filename, 
                                   std::string& content_type) {
    // Diviser les headers en lignes
    std::istringstream iss(part_headers);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Nettoyer les fins de ligne (\r)
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        // Ignorer les lignes vides
        if (line.empty()) {
            continue;
        }
        
        // Traiter Content-Disposition
        if (line.find("Content-Disposition:") == 0) {
            size_t name_pos = line.find("name=\"");
            if (name_pos != std::string::npos) {
                name_pos += 6; // Longueur de 'name="'
                size_t name_end = line.find("\"", name_pos);
                if (name_end != std::string::npos) {
                    name = line.substr(name_pos, name_end - name_pos);
                }
            }
            
            size_t filename_pos = line.find("filename=\"");
            if (filename_pos != std::string::npos) {
                filename_pos += 10; // Longueur de 'filename="'
                size_t filename_end = line.find("\"", filename_pos);
                if (filename_end != std::string::npos) {
                    filename = line.substr(filename_pos, filename_end - filename_pos);
                }
            }
        }
        // Traiter Content-Type
        else if (line.find("Content-Type:") == 0) {
            size_t ct_start = 13; // Longueur de 'Content-Type:'
            // Ignorer les espaces après "Content-Type:"
            while (ct_start < line.length() && line[ct_start] == ' ') {
                ct_start++;
            }
            content_type = line.substr(ct_start);
        }
    }
} 