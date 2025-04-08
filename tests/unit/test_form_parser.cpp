#include "http/HttpRequest.hpp"
#include "utils/Common.hpp"
#include <iostream>
#include <cassert>

void testUrlEncodedForm() {
    LOG_INFO("\n=== Test: URL-encoded form ===");
    
    std::string request = 
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 38\r\n"
        "\r\n"
        "username=john_doe&password=secret%21&age=25";
    
    HttpRequest req;
    bool result = req.parse(request);
    
    assert(result);
    assert(req.getMethod() == "POST");
    assert(req.getUri() == "/submit");
    
    LOG_INFO("Form values:");
    LOG_INFO("  username: " << req.getFormValue("username"));
    LOG_INFO("  password: " << req.getFormValue("password"));
    LOG_INFO("  age: " << req.getFormValue("age"));
    
    assert(req.getFormValue("username") == "john_doe");
    assert(req.getFormValue("password") == "secret!"); // %21 décodé en !
    assert(req.getFormValue("age") == "25");
    
    LOG_SUCCESS("URL-encoded form test passed!");
}

void testMultipartFormData() {
    LOG_INFO("\n=== Test: Multipart form data ===");
    LOG_INFO("NOTE: Multipart/form-data parsing has been temporarily disabled");
    LOG_INFO("This test is skipped and will be re-enabled when the feature is reimplemented");
    
    // Test is skipped as multipart/form-data parsing is temporarily disabled
    
    LOG_SUCCESS("Multipart form data test skipped (functionality disabled)!");
}

void testContentLengthValidation() {
    LOG_INFO("\n=== Test: Content-Length validation ===");
    
    // Créer une requête avec un Content-Length énorme (> MAX_BODY_SIZE)
    std::string request = 
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: 1073741824\r\n" // 1 Go
        "\r\n"
        "sample_data";
    
    HttpRequest req;
    bool result = req.parse(request);
    
    // La validation devrait échouer à cause du Content-Length trop grand
    assert(!result);
    
    LOG_SUCCESS("Content-Length validation test passed!");
}

int main() {
    LOG_INFO("Starting Form Parser tests...");
    
    try {
        testUrlEncodedForm();
        testMultipartFormData();
        testContentLengthValidation();
        
        LOG_SUCCESS("\nAll tests completed! (Note: multipart/form-data test was skipped due to temporary feature removal)");
        return 0;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Test failed: " << e.what());
        return 1;
    }
} 