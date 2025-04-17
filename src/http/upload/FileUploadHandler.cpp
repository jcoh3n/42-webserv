#include "http/upload/FileUploadHandler.hpp"
#include "utils/Common.hpp"
#include <fstream>
#include <sstream>

bool FileUploadHandler::handleFileUpload(const UploadedFile& file) {
    if (!config.ensureUploadDirectory()) {
        LOG_ERROR("Failed to ensure upload directory exists");
        return false;
    }
    
    if (!checkFileSize(file.data)) {
        LOG_ERROR("File size exceeds maximum allowed size");
        return false;
    }
    
    if (saveFile(file.filename, file.data)) {
        LOG_SUCCESS("File uploaded successfully: " << file.filename);
        return true;
    }
    
    return false;
}

HttpResponse FileUploadHandler::createUploadResponse(int success_count, int total_count) {
    HttpResponse response;
    std::ostringstream html;
    
    html << "<html><body><h1>Upload Results</h1>";
    
    if (success_count == total_count) {
        response.setStatus(201); // Created
        html << "<p>All files were uploaded successfully!</p>";
    }
    else if (success_count > 0) {
        response.setStatus(206); // Partial Content
        html << "<p>Some files were uploaded successfully.</p>";
    }
    else {
        response.setStatus(500); // Internal Server Error
        html << "<p>No files were uploaded successfully.</p>";
    }
    
    html << "<ul>";
    html << "<li>Successfully uploaded: " << success_count << "/" << total_count << "</li>";
    html << "</ul>";
    html << "</body></html>";
    
    response.setBody(html.str(), "text/html");
    return response;
}

bool FileUploadHandler::checkFileSize(const std::string& content) const {
    return content.size() <= config.getMaxFileSize();
}

bool FileUploadHandler::saveFile(const std::string& filename, const std::string& content) const {
    std::string file_path = config.getUploadDirectory() + filename;
    std::ofstream file_out(file_path.c_str(), std::ios::binary);
    
    if (!file_out) {
        LOG_ERROR("Failed to open file for writing: " << file_path);
        return false;
    }
    
    file_out.write(content.data(), content.size());
    
    if (file_out.fail()) {
        LOG_ERROR("Failed to write file content: " << file_path);
        return false;
    }
    
    file_out.close();
    LOG_SUCCESS("File saved successfully: " << file_path);
    return true;
} 