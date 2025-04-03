#include "config/ConfigParser.hpp"
#include "utils/Common.hpp"
#include <iostream>
#include <cassert>
#include <sstream>
#include <fstream>

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

void test_basic_config() {
    LOG_INFO("Test: Configuration de base");
    
    std::string config_content = 
        "[server]\n"
        "port=8080\n"
        "host=0.0.0.0\n"
        "server_name=localhost\n"
        "root=./www\n"
        "client_max_body_size=1M\n"
        "\n"
        "[location /]\n"
        "allowed_methods=GET POST\n"
        "index=index.html\n"
        "\n"
        "[location /upload]\n"
        "allowed_methods=POST\n"
        "upload_directory=./uploads\n";
    
    std::string filename = createTempConfigFile(config_content);
    HTTP::ConfigParser parser;
    HTTP::WebservConfig config = parser.parseFile(filename);
    
    // Vérifier que nous avons 1 serveur
    assert(config.servers.size() == 1);
    
    // Vérifier les paramètres du serveur
    HTTP::ServerConfig server = config.servers[0];
    assert(server.port == 8080);
    assert(server.host == "0.0.0.0");
    assert(server.server_names.size() == 1);
    assert(server.server_names[0] == "localhost");
    assert(server.root_directory == "./www");
    assert(server.client_max_body_size == 1048576); // 1MB
    
    // Vérifier les locations
    assert(server.locations.size() == 2);
    assert(server.locations.find("/") != server.locations.end());
    assert(server.locations.find("/upload") != server.locations.end());
    
    // Vérifier la location /
    HTTP::LocationConfig root_location = server.locations["/"];
    assert(root_location.allowed_methods.size() == 2);
    assert(root_location.allowed_methods[0] == "GET");
    assert(root_location.allowed_methods[1] == "POST");
    assert(root_location.index_files.size() == 1);
    assert(root_location.index_files[0] == "index.html");
    
    // Vérifier la location /upload
    HTTP::LocationConfig upload_location = server.locations["/upload"];
    assert(upload_location.allowed_methods.size() == 1);
    assert(upload_location.allowed_methods[0] == "POST");
    assert(upload_location.upload_directory == "./uploads");
    
    LOG_SUCCESS("Test de configuration de base réussi");
}

void test_multiple_servers() {
    LOG_INFO("Test: Plusieurs serveurs");
    
    std::string config_content = 
        "[server]\n"
        "port=8080\n"
        "server_name=site1.local\n"
        "root=./www/site1\n"
        "\n"
        "[server]\n"
        "port=8080\n"
        "server_name=site2.local\n"
        "root=./www/site2\n"
        "\n"
        "[server]\n"
        "port=8081\n"
        "server_name=site3.local\n"
        "root=./www/site3\n";
    
    std::string filename = createTempConfigFile(config_content);
    HTTP::ConfigParser parser;
    HTTP::WebservConfig config = parser.parseFile(filename);
    
    // Vérifier que nous avons 3 serveurs
    assert(config.servers.size() == 3);
    
    // Vérifier que les serveurs sont correctement configurés
    assert(config.servers[0].server_names[0] == "site1.local");
    assert(config.servers[0].port == 8080);
    assert(config.servers[0].root_directory == "./www/site1");
    
    assert(config.servers[1].server_names[0] == "site2.local");
    assert(config.servers[1].port == 8080);
    assert(config.servers[1].root_directory == "./www/site2");
    
    assert(config.servers[2].server_names[0] == "site3.local");
    assert(config.servers[2].port == 8081);
    assert(config.servers[2].root_directory == "./www/site3");
    
    // Tester selectVirtualServer
    const HTTP::ServerConfig& selected1 = HTTP::selectVirtualServer(config, "site1.local", 8080);
    assert(selected1.server_names[0] == "site1.local");
    
    const HTTP::ServerConfig& selected2 = HTTP::selectVirtualServer(config, "site2.local", 8080);
    assert(selected2.server_names[0] == "site2.local");
    
    const HTTP::ServerConfig& selected3 = HTTP::selectVirtualServer(config, "site3.local", 8081);
    assert(selected3.server_names[0] == "site3.local");
    
    LOG_SUCCESS("Test de plusieurs serveurs réussi");
}

void test_location_selection() {
    LOG_INFO("Test: Sélection de location");
    
    std::string config_content = 
        "[server]\n"
        "port=8080\n"
        "server_name=localhost\n"
        "root=./www\n"
        "\n"
        "[location /]\n"
        "allowed_methods=GET\n"
        "index=index.html\n"
        "\n"
        "[location /api]\n"
        "allowed_methods=GET POST\n"
        "index=api.html\n"
        "\n"
        "[location /api/v1]\n"
        "allowed_methods=GET POST DELETE\n"
        "index=v1.html\n";
    
    std::string filename = createTempConfigFile(config_content);
    HTTP::ConfigParser parser;
    HTTP::WebservConfig config = parser.parseFile(filename);
    
    // Obtenir le serveur
    const HTTP::ServerConfig& server = config.servers[0];
    
    // Tester la sélection de location
    const HTTP::LocationConfig& loc1 = HTTP::selectLocation(server, "/");
    assert(loc1.index_files.size() == 1);
    assert(loc1.index_files[0] == "index.html");
    assert(loc1.allowed_methods.size() == 1);
    
    const HTTP::LocationConfig& loc2 = HTTP::selectLocation(server, "/api");
    assert(loc2.index_files.size() == 1);
    assert(loc2.index_files[0] == "api.html");
    assert(loc2.allowed_methods.size() == 2);
    
    const HTTP::LocationConfig& loc3 = HTTP::selectLocation(server, "/api/v1");
    assert(loc3.index_files.size() == 1);
    assert(loc3.index_files[0] == "v1.html");
    assert(loc3.allowed_methods.size() == 3);
    
    // Tester la correspondance de préfixe
    const HTTP::LocationConfig& loc4 = HTTP::selectLocation(server, "/api/v1/users");
    assert(loc4.index_files.size() == 1);
    assert(loc4.index_files[0] == "v1.html");  // Doit correspondre à /api/v1
    
    const HTTP::LocationConfig& loc5 = HTTP::selectLocation(server, "/other");
    assert(loc5.index_files.size() == 1);
    assert(loc5.index_files[0] == "index.html");  // Doit correspondre à /
    
    LOG_SUCCESS("Test de sélection de location réussi");
}

void test_error_cases() {
    LOG_INFO("Test: Cas d'erreur");
    
    // Test: Directive en dehors d'une section
    std::string config1 = 
        "port=8080\n"
        "[server]\n"
        "host=0.0.0.0\n";
    
    std::string filename1 = createTempConfigFile(config1);
    HTTP::ConfigParser parser;
    
    try {
        parser.parseFile(filename1);
        assert(false); // Ne devrait pas arriver ici
    } catch (const std::exception& e) {
        LOG_INFO("Exception attendue reçue: " << e.what());
    }
    
    // Test: Directive invalide (sans =)
    std::string config2 = 
        "[server]\n"
        "port 8080\n";
    
    std::string filename2 = createTempConfigFile(config2);
    
    try {
        parser.parseFile(filename2);
        assert(false); // Ne devrait pas arriver ici
    } catch (const std::exception& e) {
        LOG_INFO("Exception attendue reçue: " << e.what());
    }
    
    // Test: Directive inconnue
    std::string config3 = 
        "[server]\n"
        "port=8080\n"
        "unknown_directive=value\n";
    
    std::string filename3 = createTempConfigFile(config3);
    
    try {
        parser.parseFile(filename3);
        assert(false); // Ne devrait pas arriver ici
    } catch (const std::exception& e) {
        LOG_INFO("Exception attendue reçue: " << e.what());
    }
    
    // Test: Section invalide
    std::string config4 = 
        "[unknown_section]\n"
        "key=value\n";
    
    std::string filename4 = createTempConfigFile(config4);
    
    try {
        parser.parseFile(filename4);
        assert(false); // Ne devrait pas arriver ici
    } catch (const std::exception& e) {
        LOG_INFO("Exception attendue reçue: " << e.what());
    }
    
    // Test: Location en dehors d'un serveur
    std::string config5 = 
        "[location /]\n"
        "allowed_methods=GET\n";
    
    std::string filename5 = createTempConfigFile(config5);
    
    try {
        parser.parseFile(filename5);
        assert(false); // Ne devrait pas arriver ici
    } catch (const std::exception& e) {
        LOG_INFO("Exception attendue reçue: " << e.what());
    }
    
    LOG_SUCCESS("Test des cas d'erreur réussi");
}

int main() {
    LOG_INFO("=== Test du parseur de configuration ===");
    
    try {
        test_basic_config();
        test_multiple_servers();
        test_location_selection();
        test_error_cases();
        LOG_SUCCESS("Tous les tests ont réussi!");
        return 0;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Erreur de test: " << e.what());
        return 1;
    }
} 