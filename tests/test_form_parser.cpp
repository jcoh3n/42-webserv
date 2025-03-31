#include "http/HttpRequest.hpp"
#include <iostream>
#include <cassert>

void testUrlEncodedForm() {
    std::cout << "\n=== Test: URL-encoded form ===\n";
    
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
    
    std::cout << "Form values:\n";
    std::cout << "  username: " << req.getFormValue("username") << std::endl;
    std::cout << "  password: " << req.getFormValue("password") << std::endl;
    std::cout << "  age: " << req.getFormValue("age") << std::endl;
    
    assert(req.getFormValue("username") == "john_doe");
    assert(req.getFormValue("password") == "secret!"); // %21 décodé en !
    assert(req.getFormValue("age") == "25");
    
    std::cout << "URL-encoded form test passed!\n";
}

void testMultipartFormData() {
    std::cout << "\n=== Test: Multipart form data ===\n";
    
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
    
    std::cout << "Form values:\n";
    std::cout << "  username: " << req.getFormValue("username") << std::endl;
    
    assert(req.getFormValue("username") == "john_doe");
    assert(req.hasUploadedFile("file"));
    
    const UploadedFile* file = req.getUploadedFile("file");
    assert(file != NULL);
    
    std::cout << "Uploaded file:\n";
    std::cout << "  Name: " << file->name << std::endl;
    std::cout << "  Filename: " << file->filename << std::endl;
    std::cout << "  Content-Type: " << file->content_type << std::endl;
    std::cout << "  Size: " << file->data.size() << " bytes\n";
    std::cout << "  Content: " << file->data << std::endl;
    
    assert(file->name == "file");
    assert(file->filename == "test.txt");
    assert(file->content_type == "text/plain");
    assert(file->data == "This is the content of the file");
    
    std::cout << "Multipart form data test passed!\n";
}

void testContentLengthValidation() {
    std::cout << "\n=== Test: Content-Length validation ===\n";
    
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
    
    std::cout << "Content-Length validation test passed!\n";
}

int main() {
    std::cout << "Starting Form Parser tests...\n";
    
    try {
        testUrlEncodedForm();
        testMultipartFormData();
        testContentLengthValidation();
        
        std::cout << "\nAll tests passed successfully!\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
} 