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
    
    std::string boundary = "----WebKitFormBoundaryABC123";
    std::string request = 
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n"
        "Content-Length: 400\r\n"
        "\r\n"
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"username\"\r\n"
        "\r\n"
        "john_doe\r\n"
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "This is the content of the file\r\n"
        "--" + boundary + "--\r\n";
    
    HttpRequest req;
    bool result = req.parse(request);
    
    assert(result);
    assert(req.getMethod() == "POST");
    assert(req.getUri() == "/upload");
    
    LOG_INFO("Form values:");
    LOG_INFO("  username: " << req.getFormValue("username"));
    
    assert(req.getFormValue("username") == "john_doe");
    assert(req.hasUploadedFile("file"));
    
    const UploadedFile* file = req.getUploadedFile("file");
    assert(file != NULL);
    
    LOG_INFO("Uploaded file:");
    LOG_INFO("  Name: " << file->name);
    LOG_INFO("  Filename: " << file->filename);
    LOG_INFO("  Content-Type: " << file->content_type);
    LOG_INFO("  Size: " << file->data.size() << " bytes");
    LOG_INFO("  Content: " << file->data);
    
    assert(file->name == "file");
    assert(file->filename == "test.txt");
    assert(file->content_type == "text/plain");
    assert(file->data == "This is the content of the file");
    
    LOG_SUCCESS("Multipart form data test passed!");
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
        
        LOG_SUCCESS("\nAll tests passed successfully!");
        return 0;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Test failed: " << e.what());
        return 1;
    }
} 