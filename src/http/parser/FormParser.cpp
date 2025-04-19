#include "http/parser/FormParser.hpp"
#include "http/utils/HttpUtils.hpp"
#include "utils/Common.hpp"
#include <sstream>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <cstring>

// Implémentation de BoundaryExtractor
std::string BoundaryExtractor::extract(const std::string& content_type) {
    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string::npos) {
        return "";
    }
    
    std::string boundary = content_type.substr(boundary_pos + 9);
    return clean(boundary);
}

std::string BoundaryExtractor::clean(const std::string& boundary) {
    std::string result = boundary;
    trimWhitespace(result);
    removeQuotes(result);
    removeSemicolonAndAfter(result);
    return result;
}

void BoundaryExtractor::removeQuotes(std::string& boundary) {
    if (boundary.length() >= 2 && boundary[0] == '"') {
        boundary = boundary.substr(1);
    }
    if (boundary.length() >= 1 && boundary[boundary.length() - 1] == '"') {
        boundary = boundary.substr(0, boundary.length() - 1);
    }
}

void BoundaryExtractor::removeSemicolonAndAfter(std::string& boundary) {
    size_t semicolon_pos = boundary.find(';');
    if (semicolon_pos != std::string::npos) {
        boundary = boundary.substr(0, semicolon_pos);
    }
}

void BoundaryExtractor::trimWhitespace(std::string& boundary) {
    boundary.erase(0, boundary.find_first_not_of(" \t"));
    boundary.erase(boundary.find_last_not_of(" \t") + 1);
}

// Implémentation de MultipartHeaderParser
MultipartHeaderParser::HeaderInfo MultipartHeaderParser::parse(const std::string& headers) {
    HeaderInfo info;
    std::istringstream iss(headers);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        if (line.empty()) continue;
        
        size_t colon = line.find(": ");
        if (colon != std::string::npos) {
            std::string header_name = line.substr(0, colon);
            std::string header_value = line.substr(colon + 2);
            
            if (header_name == "Content-Disposition") {
                parseContentDisposition(header_value, info);
            }
            else if (header_name == "Content-Type") {
                parseContentType(header_value, info);
            }
        }
    }
    
    return info;
}

void MultipartHeaderParser::parseContentDisposition(const std::string& header_value, HeaderInfo& info) {
    size_t name_pos = header_value.find("name=\"");
    if (name_pos != std::string::npos) {
        name_pos += 6;
        size_t name_end = header_value.find("\"", name_pos);
        if (name_end != std::string::npos) {
            info.name = header_value.substr(name_pos, name_end - name_pos);
        }
    }
    
    size_t filename_pos = header_value.find("filename=\"");
    if (filename_pos != std::string::npos) {
        filename_pos += 10;
        size_t filename_end = header_value.find("\"", filename_pos);
        if (filename_end != std::string::npos) {
            info.filename = header_value.substr(filename_pos, filename_end - filename_pos);
        }
    }
}

void MultipartHeaderParser::parseContentType(const std::string& header_value, HeaderInfo& info) {
    info.content_type = header_value;
}

// Implémentation de FormParser
void FormParser::parseUrlEncoded(const std::string& body, FormData& form_data) {
    std::istringstream iss(body);
    std::string pair;
    
    while (std::getline(iss, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos == std::string::npos) continue;
        
        std::string key = pair.substr(0, eq_pos);
        std::string value = pair.substr(eq_pos + 1);
        
        form_data.addFormValue(HttpUtils::urlDecode(key), HttpUtils::urlDecode(value));
    }
}

void FormParser::parseMultipart(const std::string& body, const std::string& boundary, FormData& form_data) {
    std::string full_boundary = "--" + boundary;
    std::string end_boundary = full_boundary + "--";
    
    size_t pos = 0;
    size_t boundary_pos;
    
    boundary_pos = body.find(full_boundary);
    if (boundary_pos == std::string::npos) {
        LOG_ERROR("No boundary found in body");
        return;
    }
    
    pos = boundary_pos + full_boundary.length();
    
    while (pos < body.length()) {
        if (body.substr(boundary_pos, end_boundary.length()) == end_boundary) {
            break;
        }
        
        boundary_pos = body.find(full_boundary, pos);
        if (boundary_pos == std::string::npos) {
            boundary_pos = body.length();
        }
        
        std::string part = body.substr(pos, boundary_pos - pos);
        
        while (part.length() >= 2 && part.substr(0, 2) == "\r\n") {
            part = part.substr(2);
        }
        
        while (part.length() >= 2 && part.substr(part.length() - 2) == "\r\n") {
            part = part.substr(0, part.length() - 2);
        }
        
        if (!part.empty()) {
            parseMultipartPart(part, form_data);
        }
        
        pos = boundary_pos + full_boundary.length();
    }
}

void FormParser::parseMultipartPart(const std::string& part, FormData& form_data) {
    size_t header_end = part.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        LOG_ERROR("Invalid multipart part: no header separation");
        return;
    }
    
    std::string headers = part.substr(0, header_end);
    std::string content = part.substr(header_end + 4);
    
    if (content.length() >= 2 && content.substr(content.length() - 2) == "\r\n") {
        content = content.substr(0, content.length() - 2);
    }
    
    MultipartHeaderParser::HeaderInfo info = MultipartHeaderParser::parse(headers);
    
    if (!info.filename.empty()) {
        UploadedFile file;
        file.name = info.name;
        file.filename = info.filename;
        file.content_type = info.content_type;
        file.data = content;
        
        if (saveUploadedFile(file, content)) {
            form_data.addUploadedFile(info.name, file);
            LOG_UPLOAD("File saved: " << info.filename);
        }
    }
    else if (!info.name.empty()) {
        form_data.addFormValue(info.name, content);
    }
}

bool FormParser::saveUploadedFile(const UploadedFile& file, const std::string& content) {
    std::string upload_dir = "./www/uploads/";
    
    struct stat info;
    if (stat(upload_dir.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR)) {
        LOG_WARNING("Upload directory does not exist, creating: " << upload_dir);
        if (mkdir(upload_dir.c_str(), 0755) != 0) {
            LOG_ERROR("Failed to create upload directory: " << upload_dir);
            return false;
        }
    }
    
    std::string file_path = upload_dir + file.filename;
    std::ofstream file_out(file_path.c_str(), std::ios::binary);
    if (!file_out) {
        LOG_ERROR("Failed to save uploaded file: " << file_path);
        return false;
    }
    
    file_out.write(content.data(), content.size());
    file_out.close();
    
    return true;
} 