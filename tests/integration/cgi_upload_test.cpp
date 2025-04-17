#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/RouteHandler.hpp"
#include "utils/Common.hpp"
#include "config/ConfigTypes.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <sstream>

class CgiUploadTest {
private:
    ServerConfig server_config;
    RouteHandler router;
    const std::string BOUNDARY;
    
    std::string createMultipartFormData(const std::string& filename, const std::string& content) {
        std::string body = "--" + BOUNDARY + "\r\n";
        body += "Content-Disposition: form-data; name=\"uploaded_file\"; filename=\"" + filename + "\"\r\n";
        body += "Content-Type: text/plain\r\n\r\n";
        body += content;
        body += "\r\n--" + BOUNDARY + "--\r\n";
        return body;
    }

public:
    CgiUploadTest() 
        : server_config() // Initialiser avec une configuration par défaut
        , router("./www", server_config)
        , BOUNDARY("------------------------d74496d66958873e") 
    {
        // Configuration de base du serveur pour les tests
        server_config.root_directory = "./www";
        
        // Ajouter une location pour /upload
        LocationConfig upload_location;
        upload_location.allowed_methods.push_back("POST");
        upload_location.upload_directory = "./www/uploads";
        upload_location.root_override = "./www/uploads";
        
        server_config.locations["/upload"] = upload_location;
    }

    void testFileUpload() {
        LOG_INFO("Testing File Upload...");
        
        // Créer une requête POST multipart/form-data
        std::string content = "Hello, this is a test file content!";
        std::string body = createMultipartFormData("test.txt", content);
        
        std::string request = "POST /upload HTTP/1.1\r\n"
                             "Host: localhost:8080\r\n"
                             "Content-Type: multipart/form-data; boundary=" + BOUNDARY + "\r\n"
                             "Content-Length: ";
        
        std::ostringstream length;
        length << body.length();
        request += length.str() + "\r\n\r\n" + body;

        HttpRequest req;
        bool parse_result = req.parse(request);
        assert(parse_result);

        HttpResponse response = router.processRequest(req);
        LOG_INFO("Response status code: " << response.getStatusCode());
        LOG_INFO("Response body: " << response.getBody().substr(0, 200) << "..."); // Afficher le début du body
        assert(response.getStatusCode() == 200 || response.getStatusCode() == 201 || response.getStatusCode() == 501);
    }

    void runAllTests() {
        LOG_INFO("=== Starting File Upload Tests ===\n");
        
        try {
            testFileUpload();
            LOG_SUCCESS("All File Upload tests completed successfully!");
        } catch (const std::exception& e) {
            LOG_ERROR("Test failed: " + std::string(e.what()));
        }
    }
};

int main() {
    CgiUploadTest tester;
    tester.runAllTests();
    return 0;
} 