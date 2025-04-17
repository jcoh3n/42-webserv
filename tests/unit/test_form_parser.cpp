#include "http/parser/FormParser.hpp"
#include "http/upload/FileUploadHandler.hpp"
#include "http/upload/UploadConfig.hpp"
#include "utils/Common.hpp"
#include <iostream>
#include <cassert>
#include <sstream>

void testUrlEncodedForm() {
    LOG_INFO("\n=== Test: URL-encoded form ===");
    
    std::string body = "username=john_doe&password=secret%21&age=25";
    FormData form_data;
    
    FormParser::parseUrlEncoded(body, form_data);
    
    assert(form_data.getFormValue("username") == "john_doe");
    assert(form_data.getFormValue("password") == "secret!");
    assert(form_data.getFormValue("age") == "25");
    
    LOG_SUCCESS("URL-encoded form test passed!");
}

void testBoundaryExtractor() {
    LOG_INFO("\n=== Test: Boundary Extractor ===");
    
    // Test avec guillemets
    std::string content_type1 = "multipart/form-data; boundary=\"abc123\"";
    assert(BoundaryExtractor::extract(content_type1) == "abc123");
    
    // Test sans guillemets
    std::string content_type2 = "multipart/form-data; boundary=abc123";
    assert(BoundaryExtractor::extract(content_type2) == "abc123");
    
    // Test avec paramètres supplémentaires
    std::string content_type3 = "multipart/form-data; boundary=abc123; charset=utf-8";
    assert(BoundaryExtractor::extract(content_type3) == "abc123");
    
    LOG_SUCCESS("Boundary extractor test passed!");
}

void testMultipartFormData() {
    LOG_INFO("\n=== Test: Multipart form data ===");
    
    std::string boundary = "------------------------d74496d66958873e";
    std::string body = 
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"field1\"\r\n"
        "\r\n"
        "value1\r\n"
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"file1\"; filename=\"test.txt\"\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello, World!\r\n"
        "--" + boundary + "--\r\n";
    
    FormData form_data;
    FormParser::parseMultipart(body, boundary, form_data);
    
    // Vérifier le champ normal
    assert(form_data.getFormValue("field1") == "value1");
    
    // Vérifier le fichier
    assert(form_data.hasUploadedFile("file1"));
    const UploadedFile* file = form_data.getUploadedFile("file1");
    assert(file != NULL);
    assert(file->filename == "test.txt");
    assert(file->content_type == "text/plain");
    assert(file->data == "Hello, World!");
    
    LOG_SUCCESS("Multipart form data test passed!");
}

void testFileUploadHandler() {
    LOG_INFO("\n=== Test: File Upload Handler ===");
    
    // Créer un répertoire temporaire pour les tests
    std::string test_upload_dir = "./test_uploads/";
    UploadConfig config(test_upload_dir, 1024 * 1024);  // 1MB max
    FileUploadHandler handler(config);
    
    // Créer un fichier de test
    UploadedFile test_file;
    test_file.name = "test_file";
    test_file.filename = "test.txt";
    test_file.content_type = "text/plain";
    test_file.data = "Test content";
    
    // Tester l'upload
    bool result = handler.handleFileUpload(test_file);
    assert(result);
    
    // Vérifier que le fichier existe
    std::ifstream file((test_upload_dir + test_file.filename).c_str());
    assert(file.good());
    
    // Nettoyer
    system(("rm -rf " + test_upload_dir).c_str());
    
    LOG_SUCCESS("File upload handler test passed!");
}

int main() {
    LOG_INFO("Starting Form Parser and Upload tests...");
    
    try {
        testUrlEncodedForm();
        testBoundaryExtractor();
        testMultipartFormData();
        testFileUploadHandler();
        
        LOG_SUCCESS("\nAll tests passed successfully!");
        return 0;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Test failed: " << e.what());
        return 1;
    }
} 