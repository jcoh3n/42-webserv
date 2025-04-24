#include "config/ConfigParser.hpp"
#include "config/ConfigTypes.hpp"
#include "utils/Common.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>

// Fonction pour créer un fichier de configuration temporaire
std::string createTempConfigFile(const std::string& content) {
    std::string filename = "/tmp/webserv_test_config.conf";
    std::ofstream file(filename.c_str());
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create temp config file");
    }
    
    file << content;
    file.close();
    
    return filename;
}

// Test de configuration basique
void test_basic_config() {
    LOG_INFO("Test de configuration basique...");
    
    // Créer un fichier de configuration temporaire
    const char* filename = "test_basic.conf";
    std::ofstream file(filename);
    file << "server {\n"
         << "    port=8080\n"
         << "    host=127.0.0.1\n"
         << "    server_name=localhost\n"
         << "    root=/var/www/html\n"
         << "    error_page=404 /404.html\n"
         << "    error_page=500 /500.html\n"
         << "\n"
         << "    location / {\n"
         << "        allowed_methods=GET POST\n"
         << "        index=index.html\n"
         << "        autoindex=off\n"
         << "        client_max_body_size=20M\n"
         << "    }\n"
         << "\n"
         << "    location /file-upload {\n"
         << "        allowed_methods=POST\n"
         << "        client_max_body_size=50M\n"
         << "    }\n"
         << "}\n";
    file.close();

    // Parser la configuration
    ConfigParser parser;
    WebservConfig config = parser.parseFile(filename);

    // Vérifier les paramètres de base
    assert(config.servers.size() == 1);

    // Vérifier les paramètres du serveur
    ServerConfig server = config.servers[0];
    assert(server.port == 8080);
    assert(server.host == "127.0.0.1");
    assert(server.server_names.size() == 1);
    assert(server.server_names[0] == "localhost");
    assert(server.root_directory == "/var/www/html");
    assert(server.error_pages.size() == 2);
    assert(server.error_pages[404] == "/404.html");
    assert(server.error_pages[500] == "/500.html");

    // Vérifier la location racine
    LocationConfig root_location = server.locations["/"];
    assert(root_location.allowed_methods.size() == 2);
    assert(root_location.allowed_methods[0] == "GET");
    assert(root_location.allowed_methods[1] == "POST");
    assert(root_location.index_files.size() == 1);
    assert(root_location.index_files[0] == "index.html");
    assert(!root_location.autoindex);
    assert(root_location.client_max_body_size == 20 * 1024 * 1024); // 20M

    // Vérifier la location /upload
    LocationConfig upload_location = server.locations["/file-upload"];
    assert(upload_location.allowed_methods.size() == 1);
    assert(upload_location.allowed_methods[0] == "POST");
    assert(upload_location.client_max_body_size == 50 * 1024 * 1024); // 50M

    // Nettoyer
    std::remove(filename);
    LOG_SUCCESS("Test de configuration basique réussi!");
}

// Test de configuration avec plusieurs serveurs
void test_multiple_servers() {
    LOG_INFO("Test de configuration avec plusieurs serveurs...");
    
    const char* filename = "test_multiple.conf";
    std::ofstream file(filename);
    file << "server {\n"
         << "    port=8080\n"
         << "    host=127.0.0.1\n"
         << "    server_name=site1.local\n"
         << "}\n"
         << "\n"
         << "server {\n"
         << "    port=8080\n"
         << "    host=127.0.0.1\n"
         << "    server_name=site2.local\n"
         << "}\n"
         << "\n"
         << "server {\n"
         << "    port=8081\n"
         << "    host=127.0.0.1\n"
         << "    server_name=site3.local\n"
         << "}\n";
    file.close();

    // Parser la configuration
    ConfigParser parser;
    WebservConfig config = parser.parseFile(filename);

    // Vérifier le nombre de serveurs
    assert(config.servers.size() == 3);

    // Tester la sélection de serveur virtuel
    const ServerConfig& selected1 = selectVirtualServer(config, "site1.local", 8080);
    assert(selected1.server_names[0] == "site1.local");

    const ServerConfig& selected2 = selectVirtualServer(config, "site2.local", 8080);
    assert(selected2.server_names[0] == "site2.local");

    const ServerConfig& selected3 = selectVirtualServer(config, "site3.local", 8081);
    assert(selected3.server_names[0] == "site3.local");

    // Nettoyer
    std::remove(filename);
    LOG_SUCCESS("Test de configuration multiple réussi!");
}

// Test de sélection de location
void test_location_selection() {
    LOG_INFO("Test de sélection de location...");
    
    const char* filename = "test_location.conf";
    std::ofstream file(filename);
    file << "server {\n"
         << "    port=8080\n"
         << "    host=127.0.0.1\n"
         << "    server_name=localhost\n"
         << "\n"
         << "    location / {\n"
         << "        allowed_methods=GET POST\n"
         << "        index=index.html\n"
         << "        client_max_body_size=1M\n"
         << "    }\n"
         << "\n"
         << "    location /api {\n"
         << "        allowed_methods=GET POST PUT DELETE\n"
         << "        index=api.html\n"
         << "        client_max_body_size=5M\n"
         << "    }\n"
         << "\n"
         << "    location /api/v1 {\n"
         << "        allowed_methods=GET POST\n"
         << "        index=v1.html\n"
         << "        client_max_body_size=10M\n"
         << "    }\n"
         << "}\n";
    file.close();

    // Parser la configuration
    ConfigParser parser;
    WebservConfig config = parser.parseFile(filename);

    // Obtenir le serveur
    const ServerConfig& server = config.servers[0];

    // Tester la sélection de location
    const LocationConfig& loc1 = selectLocation(server, "/");
    assert(loc1.index_files.size() == 1);
    assert(loc1.index_files[0] == "index.html");
    assert(loc1.allowed_methods.size() == 2);
    assert(loc1.allowed_methods[0] == "GET");
    assert(loc1.allowed_methods[1] == "POST");
    assert(loc1.client_max_body_size == 1024 * 1024); // 1M

    // Test de correspondance exacte
    const LocationConfig& loc2 = selectLocation(server, "/api");
    assert(loc2.index_files.size() == 1);
    assert(loc2.index_files[0] == "api.html");
    assert(loc2.allowed_methods.size() == 4);
    assert(loc2.client_max_body_size == 5 * 1024 * 1024); // 5M

    // Test de correspondance exacte plus profonde
    const LocationConfig& loc3 = selectLocation(server, "/api/v1");
    assert(loc3.index_files.size() == 1);
    assert(loc3.index_files[0] == "v1.html");
    assert(loc3.allowed_methods.size() == 2);
    assert(loc3.client_max_body_size == 10 * 1024 * 1024); // 10M

    // Test de correspondance préfixe
    const LocationConfig& loc4 = selectLocation(server, "/api/v1/users");
    assert(loc4.index_files.size() == 1);
    assert(loc4.index_files[0] == "v1.html");
    assert(loc4.allowed_methods.size() == 2);
    assert(loc4.client_max_body_size == 10 * 1024 * 1024); // 10M

    // Test de fallback sur la location racine
    const LocationConfig& loc5 = selectLocation(server, "/other");
    assert(loc5.index_files.size() == 1);
    assert(loc5.index_files[0] == "index.html");
    assert(loc5.allowed_methods.size() == 2);
    assert(loc5.client_max_body_size == 1024 * 1024); // 1M

    // Nettoyer
    std::remove(filename);
    LOG_SUCCESS("Test de sélection de location réussi!");
}

// Test des cas d'erreur
void test_error_cases() {
    LOG_INFO("Test des cas d'erreur...");
    
    ConfigParser parser;

    // Test 1: Fichier inexistant
    const char* filename1 = "nonexistent.conf";
    try {
        parser.parseFile(filename1);
        assert(false); // Ne devrait pas arriver
    } catch (const std::runtime_error&) {
        LOG_SUCCESS("Test fichier inexistant réussi!");
    }

    // Test 2: Syntaxe invalide
    const char* filename2 = "invalid_syntax.conf";
    std::ofstream file2(filename2);
    file2 << "server {\n"
          << "    port=8080\n"
          << "    host=127.0.0.1\n"
          << "    server_name=localhost\n"
          << "    invalid_directive\n" // Directive invalide
          << "}\n";
    file2.close();

    try {
        parser.parseFile(filename2);
        assert(false); // Ne devrait pas arriver
    } catch (const std::runtime_error&) {
        LOG_SUCCESS("Test syntaxe invalide réussi!");
    }
    std::remove(filename2);

    // Test 3: Port invalide
    const char* filename3 = "invalid_port.conf";
    std::ofstream file3(filename3);
    file3 << "server {\n"
          << "    port=99999\n" // Port invalide
          << "    host=127.0.0.1\n"
          << "    server_name=localhost\n"
          << "}\n";
    file3.close();

    try {
        parser.parseFile(filename3);
        assert(false); // Ne devrait pas arriver
    } catch (const std::runtime_error&) {
        LOG_SUCCESS("Test port invalide réussi!");
    }
    std::remove(filename3);

    // Test 4: Port manquant
    const char* filename4 = "missing_port.conf";
    std::ofstream file4(filename4);
    file4 << "server {\n"
          << "    host=127.0.0.1\n"
          << "    server_name=localhost\n"
          << "}\n";
    file4.close();

    bool caught_exception = false;
    try {
        parser.parseFile(filename4);
    } catch (const std::runtime_error&) {
        caught_exception = true;
        LOG_SUCCESS("Test port manquant réussi!");
    }
    std::remove(filename4);
    assert(caught_exception && "Expected exception for missing port was not thrown");

    // Test 5: Taille de corps invalide
    const char* filename5 = "invalid_body_size.conf";
    std::ofstream file5(filename5);
    file5 << "server {\n"
          << "    port=8080\n"
          << "    host=127.0.0.1\n"
          << "    server_name=localhost\n"
          << "    location / {\n"
          << "        allowed_methods=GET POST\n"
          << "        client_max_body_size=invalid\n" // Taille invalide
          << "    }\n"
          << "}\n";
    file5.close();

    try {
        parser.parseFile(filename5);
        assert(false); // Ne devrait pas arriver
    } catch (const std::runtime_error&) {
        LOG_SUCCESS("Test taille de corps invalide réussi!");
    }
    std::remove(filename5);

    // Test 6: Location sans méthodes autorisées
    const char* filename6 = "missing_methods.conf";
    std::ofstream file6(filename6);
    file6 << "server {\n"
          << "    port=8080\n"
          << "    host=127.0.0.1\n"
          << "    server_name=localhost\n"
          << "    location / {\n"
          << "        client_max_body_size=1M\n"
          << "    }\n"
          << "}\n";
    file6.close();

    try {
        parser.parseFile(filename6);
        assert(false); // Ne devrait pas arriver
    } catch (const std::runtime_error&) {
        LOG_SUCCESS("Test location sans méthodes réussi!");
    }
    std::remove(filename6);
}

int main() {
    LOG_INFO("=== Tests de Configuration ===\n");
    
    try {
        test_basic_config();
        test_multiple_servers();
        test_location_selection();
        test_error_cases();
        
        LOG_SUCCESS("\nTous les tests de configuration ont réussi!");
    } catch (const std::exception& e) {
        LOG_ERROR("Test échoué: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
} 