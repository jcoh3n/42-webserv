#include "../include/http/HttpRequest.hpp"
#include <iostream>

void printRequestInfo(const HttpRequest& req) {
    std::cout << "\nParsed Request Info:" << std::endl;
    std::cout << "Method: " << req.getMethod() << std::endl;
    std::cout << "URI: " << req.getUri() << std::endl;
    std::cout << "Version: " << req.getVersion() << std::endl;
    std::cout << "Query String: " << req.getQueryString() << std::endl;
    std::cout << "\nHeaders:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = req.getHeaders().begin();
         it != req.getHeaders().end(); ++it) {
        std::cout << it->first << ": " << it->second << std::endl;
    }
    std::cout << "\nBody: " << req.getBody() << std::endl;
}

int main() {
    HttpRequest request;
    
    // Test 1: GET simple
    std::cout << "\n=== Test 1: GET simple ===" << std::endl;
    std::string get_request = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "\r\n";
    
    if (request.parse(get_request)) {
        printRequestInfo(request);
    } else {
        std::cout << "Failed to parse GET request" << std::endl;
    }
    
    // Test 2: GET avec query string
    std::cout << "\n=== Test 2: GET avec query string ===" << std::endl;
    std::string get_query_request = 
        "GET /search?name=john&age=25 HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "\r\n";
    
    if (request.parse(get_query_request)) {
        printRequestInfo(request);
    } else {
        std::cout << "Failed to parse GET request with query" << std::endl;
    }
    
    // Test 3: POST avec body
    std::cout << "\n=== Test 3: POST avec body ===" << std::endl;
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
        std::cout << "Failed to parse POST request" << std::endl;
    }
    
    // Test 4: Requête invalide
    std::cout << "\n=== Test 4: Requête invalide ===" << std::endl;
    std::string invalid_request = "INVALID /test HTTP/1.1\r\n\r\n";
    
    if (!request.parse(invalid_request)) {
        std::cout << "Successfully rejected invalid request" << std::endl;
    } else {
        std::cout << "Error: Invalid request was accepted" << std::endl;
    }
    
    return 0;
} 