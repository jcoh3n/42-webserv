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
    // This functionality has been temporarily disabled
    // The multipart/form-data parsing and file upload feature will be reimplemented in a future version
    (void)body;      // Avoid unused param warning
    (void)boundary;  // Avoid unused param warning
    (void)form_data; // Avoid unused param warning
}

void FormParser::parseMultipartPart(const std::string& part, FormData& form_data) {
    // This functionality has been temporarily disabled
    // The multipart/form-data parsing and file upload feature will be reimplemented in a future version
    (void)part;       // Avoid unused param warning
    (void)form_data;  // Avoid unused param warning
}

void FormParser::extractPartHeaders(const std::string& part_headers, 
                                   std::string& name, 
                                   std::string& filename, 
                                   std::string& content_type) {
    // This functionality has been temporarily disabled
    // The multipart/form-data parsing and file upload feature will be reimplemented in a future version
    (void)part_headers;  // Avoid unused param warning
    (void)name;          // Avoid unused param warning
    (void)filename;      // Avoid unused param warning
    (void)content_type;  // Avoid unused param warning
} 