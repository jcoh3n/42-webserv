#include "http/upload/FileUploadHandler.hpp"
#include "http/upload/UploadConfig.hpp"
#include "http/parser/FormData.hpp"
#include "utils/Common.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cstdlib>
#include <sys/stat.h>

// Fonction utilitaire pour créer un fichier de test
void createTestFile(const std::string& path, const std::string& content) {
    std::ofstream file(path.c_str());
    file << content;
    file.close();
}

// Fonction utilitaire pour vérifier si un fichier existe
bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

void testUploadConfig() {
    LOG_INFO("\n=== Test: Upload Configuration ===");
    
    // Test avec les valeurs par défaut
    UploadConfig config1;
    assert(config1.getUploadDirectory() == "./www/uploads/");
    assert(config1.getMaxFileSize() == 10 * 1024 * 1024);
    
    // Test avec des valeurs personnalisées
    UploadConfig config2("./custom_uploads/", 1024);
    assert(config2.getUploadDirectory() == "./custom_uploads/");
    assert(config2.getMaxFileSize() == 1024);
    
    // Test de création de répertoire
    system("rm -rf ./test_uploads");
    UploadConfig config3("./test_uploads/");
    assert(config3.ensureUploadDirectory());
    assert(fileExists("./test_uploads"));
    
    LOG_SUCCESS("Upload configuration tests passed!");
}

void testFileUploadHandler() {
    LOG_INFO("\n=== Test: File Upload Handler ===");
    
    // Préparer le répertoire de test
    const std::string test_dir = "./test_uploads/";
    system(("rm -rf " + test_dir).c_str());
    UploadConfig config(test_dir, 1024 * 1024);
    FileUploadHandler handler(config);
    
    // Créer un fichier de test
    UploadedFile file;
    file.name = "test_field";
    file.filename = "test.txt";
    file.content_type = "text/plain";
    file.data = "Hello, World!";
    
    // Tester l'upload
    assert(handler.handleFileUpload(file));
    assert(fileExists(test_dir + file.filename));
    
    // Vérifier le contenu du fichier
    std::ifstream uploaded_file((test_dir + file.filename).c_str());
    std::stringstream buffer;
    buffer << uploaded_file.rdbuf();
    assert(buffer.str() == file.data);
    
    // Tester la limite de taille
    UploadConfig small_config(test_dir, 5); // Limite de 5 octets
    FileUploadHandler small_handler(small_config);
    assert(!small_handler.handleFileUpload(file)); // Devrait échouer car fichier trop grand
    
    // Nettoyer
    system(("rm -rf " + test_dir).c_str());
    
    LOG_SUCCESS("File upload handler tests passed!");
}

void testUploadResponse() {
    LOG_INFO("\n=== Test: Upload Response Generation ===");
    
    // Test avec succès total
    HttpResponse response1 = FileUploadHandler::createUploadResponse(3, 3);
    assert(response1.getStatus() == 201);
    assert(response1.getHeader("Content-Type") == "text/html");
    
    // Test avec succès partiel
    HttpResponse response2 = FileUploadHandler::createUploadResponse(1, 3);
    assert(response2.getStatus() == 206);
    
    // Test avec échec total
    HttpResponse response3 = FileUploadHandler::createUploadResponse(0, 3);
    assert(response3.getStatus() == 500);
    
    LOG_SUCCESS("Upload response generation tests passed!");
}

int main() {
    LOG_INFO("Starting Upload System Tests...");
    
    try {
        testUploadConfig();
        testFileUploadHandler();
        testUploadResponse();
        
        LOG_SUCCESS("\nAll upload tests passed successfully!");
        return 0;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Test failed: " << e.what());
        return 1;
    }
} 