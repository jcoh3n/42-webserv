#include "http/HttpRequest.hpp"
#include "utils/Common.hpp"
#include <iostream>

void printRequestInfo(const HttpRequest& req) {
    LOG_INFO("\nParsed Request Info:");
    LOG_INFO("Method: " << req.getMethod());
    LOG_INFO("URI: " << req.getUri());
    LOG_INFO("Version: " << req.getVersion());
    LOG_INFO("Query String: " << req.getQueryString());
    LOG_INFO("\nHeaders:");
    for (std::map<std::string, std::string>::const_iterator it = req.getHeaders().begin();
         it != req.getHeaders().end(); ++it) {
        LOG_INFO(it->first << ": " << it->second);
    }
    LOG_INFO("\nBody: " << req.getBody());
}

int main() {
    HttpRequest request;
    
    // Test 1: GET simple
    LOG_INFO("\n=== Test 1: GET simple ===");
    std::string get_request = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "\r\n";
    
    if (request.parse(get_request)) {
        printRequestInfo(request);
    } else {
        LOG_ERROR("Failed to parse GET request");
    }
    
    // Test 2: GET avec query string
    LOG_INFO("\n=== Test 2: GET avec query string ===");
    std::string get_query_request = 
        "GET /search?name=john&age=25 HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "\r\n";
    
    if (request.parse(get_query_request)) {
        printRequestInfo(request);
    } else {
        LOG_ERROR("Failed to parse GET request with query");
    }
    
    // Test 3: POST avec body
    LOG_INFO("\n=== Test 3: POST avec body ===");
    std::string post_request = 
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 23\r\n"
        "\r\n"
        "username=john&pass=1234";
    
    if (request.parse(post_request)) {
        printRequestInfo(request);
    } else {
        LOG_ERROR("Failed to parse POST request");
    }
    
    // Test 4: Requête invalide
    LOG_INFO("\n=== Test 4: Requête invalide ===");
    std::string invalid_request = "INVALID /test HTTP/1.1\r\n\r\n";
    
    if (!request.parse(invalid_request)) {
        LOG_SUCCESS("Successfully rejected invalid request");
    } else {
        LOG_ERROR("Error: Invalid request was accepted");
    }
    
    return 0;
} 