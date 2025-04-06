#include "http/CGIHandler.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include <iostream>
#include <cassert>

void testPHPScript() {
    std::cout << "Test 1: Exécution d'un script PHP valide..." << std::endl;
    
    // Créer une requête HTTP de test
    std::string raw_request = 
        "GET /cgi-bin/test.php?param1=value1&param2=value2 HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "\r\n";

    HttpRequest request;
    bool parsed = request.parse(raw_request);
    std::cout << "Parsing de la requête : " << (parsed ? "OK" : "ÉCHEC") << std::endl;
    
    // Vérifier que la requête est correctement parsée
    std::cout << "Méthode : " << request.getMethod() << std::endl;
    std::cout << "URI : " << request.getUri() << std::endl;
    std::cout << "Query string : " << request.getQueryString() << std::endl;

    // Créer une instance de CGIHandler avec le chemin absolu
    std::string script_path = "/home/j/Desktop/GITHUB-42/42-webserv/www/cgi-bin/test.php";
    std::cout << "Création du CGIHandler avec le script : " << script_path << std::endl;
    CGIHandler handler(request, script_path, "/usr/bin/php-cgi");
    
    // Exécuter le script CGI
    std::cout << "Exécution du script CGI..." << std::endl;
    HttpResponse response = handler.executeCGI();
    
    // Vérifier que la réponse est valide
    std::cout << "Code de statut : " << response.getStatusCode() << std::endl;
    
    const std::map<std::string, std::string>& headers = response.getHeaders();
    std::cout << "En-têtes de la réponse :" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }
    
    std::cout << "Corps de la réponse :" << std::endl;
    std::cout << response.getBody() << std::endl;
    
    // Vérifier que la réponse est valide
    assert(response.getStatusCode() == 200 && "Status code should be 200");
    
    std::map<std::string, std::string>::const_iterator it = headers.find("Content-Type");
    assert(it != headers.end() && it->second == "application/json" && "Content-Type should be application/json");
    
    // Vérifier le contenu de la réponse
    std::string body = response.getBody();
    std::cout << "Corps de la réponse pour le test :" << std::endl;
    std::cout << body << std::endl;
    std::cout << "Recherche des valeurs dans le JSON..." << std::endl;

    // Recherche plus flexible qui ignore les espaces
    bool found_status = false;
    bool found_method = false;
    
    size_t status_pos = body.find("\"status\"");
    if (status_pos != std::string::npos) {
        size_t colon_pos = body.find(":", status_pos);
        if (colon_pos != std::string::npos) {
            size_t value_start = body.find("\"", colon_pos);
            size_t value_end = body.find("\"", value_start + 1);
            if (value_start != std::string::npos && value_end != std::string::npos) {
                std::string status_value = body.substr(value_start + 1, value_end - value_start - 1);
                found_status = (status_value == "success");
            }
        }
    }

    size_t method_pos = body.find("\"method\"");
    if (method_pos != std::string::npos) {
        size_t colon_pos = body.find(":", method_pos);
        if (colon_pos != std::string::npos) {
            size_t value_start = body.find("\"", colon_pos);
            size_t value_end = body.find("\"", value_start + 1);
            if (value_start != std::string::npos && value_end != std::string::npos) {
                std::string method_value = body.substr(value_start + 1, value_end - value_start - 1);
                found_method = (method_value == "GET");
            }
        }
    }

    if (!found_status) {
        std::cout << "ERREUR: Status 'success' non trouvé dans le JSON" << std::endl;
    }
    assert(found_status && "Response should contain success status");

    if (!found_method) {
        std::cout << "ERREUR: Méthode 'GET' non trouvée dans le JSON" << std::endl;
    }
    assert(found_method && "Response should contain GET method");
    
    std::cout << "Test 1: OK!" << std::endl;
}

void testInvalidScript() {
    std::cout << "Test 2: Test avec un script invalide..." << std::endl;
    
    // Créer une requête HTTP de test
    std::string raw_request = 
        "GET /cgi-bin/nonexistent.php HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "\r\n";

    HttpRequest request;
    bool parsed = request.parse(raw_request);
    std::cout << "Parsing de la requête : " << (parsed ? "OK" : "ÉCHEC") << std::endl;
    
    // Créer une instance de CGIHandler avec un script inexistant
    std::string script_path = "/home/j/Desktop/GITHUB-42/42-webserv/www/cgi-bin/nonexistent.php";
    std::cout << "Création du CGIHandler avec un script inexistant : " << script_path << std::endl;
    CGIHandler handler(request, script_path, "/usr/bin/php-cgi");
    
    // Exécuter le script CGI
    std::cout << "Exécution du script CGI..." << std::endl;
    HttpResponse response = handler.executeCGI();
    
    // Vérifier que la réponse est une erreur 404
    std::cout << "Code de statut : " << response.getStatusCode() << std::endl;
    assert(response.getStatusCode() == 404 && "Status code should be 404 for non-existent script");
    
    std::cout << "Test 2: OK!" << std::endl;
}

void testPOSTRequest() {
    std::cout << "Test 3: Test d'une requête POST..." << std::endl;
    
    std::string raw_request = 
        "POST /cgi-bin/test.php HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 23\r\n"
        "\r\n"
        "data1=value1&data2=value2";

    HttpRequest request;
    bool parsed = request.parse(raw_request);
    std::cout << "Parsing de la requête : " << (parsed ? "OK" : "ÉCHEC") << std::endl;
    
    std::string script_path = "/home/j/Desktop/GITHUB-42/42-webserv/www/cgi-bin/test.php";
    CGIHandler handler(request, script_path, "/usr/bin/php-cgi");
    
    HttpResponse response = handler.executeCGI();
    
    assert(response.getStatusCode() == 200 && "Status code should be 200");
    
    const std::map<std::string, std::string>& headers = response.getHeaders();
    std::map<std::string, std::string>::const_iterator it = headers.find("Content-Type");
    assert(it != headers.end() && it->second == "application/json");
    
    std::string body = response.getBody();
    assert(body.find("\"method\":\"POST\"") != std::string::npos);
    assert(body.find("\"data1\":\"value1\"") != std::string::npos);
    
    std::cout << "Test 3: OK!" << std::endl;
}

void testEnvironmentVariables() {
    std::cout << "Test 4: Test des variables d'environnement..." << std::endl;
    
    std::string raw_request = 
        "GET /cgi-bin/test.php HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "X-Custom-Header: TestValue\r\n"
        "\r\n";

    HttpRequest request;
    request.parse(raw_request);
    
    std::string script_path = "/home/j/Desktop/GITHUB-42/42-webserv/www/cgi-bin/test.php";
    CGIHandler handler(request, script_path, "/usr/bin/php-cgi");
    
    HttpResponse response = handler.executeCGI();
    
    std::string body = response.getBody();
    assert(body.find("\"HTTP_X_CUSTOM_HEADER\":\"TestValue\"") != std::string::npos);
    assert(body.find("\"GATEWAY_INTERFACE\":\"CGI/1.1\"") != std::string::npos);
    assert(body.find("\"SERVER_SOFTWARE\":\"webserv/1.0\"") != std::string::npos);
    
    std::cout << "Test 4: OK!" << std::endl;
}

void testScriptErrors() {
    std::cout << "Test 5: Test des erreurs de script..." << std::endl;
    
    // Test avec un script sans permissions d'exécution
    std::string raw_request = "GET /cgi-bin/no_exec.php HTTP/1.1\r\n\r\n";
    HttpRequest request;
    request.parse(raw_request);
    
    std::string script_path = "/home/j/Desktop/GITHUB-42/42-webserv/www/cgi-bin/no_exec.php";
    // Créer un fichier sans permissions d'exécution
    system("touch /home/j/Desktop/GITHUB-42/42-webserv/www/cgi-bin/no_exec.php");
    system("chmod 644 /home/j/Desktop/GITHUB-42/42-webserv/www/cgi-bin/no_exec.php");
    
    CGIHandler handler(request, script_path, "/usr/bin/php-cgi");
    HttpResponse response = handler.executeCGI();
    
    assert(response.getStatusCode() == 403 && "Should return 403 for non-executable script");
    
    // Nettoyer
    system("rm /home/j/Desktop/GITHUB-42/42-webserv/www/cgi-bin/no_exec.php");
    
    std::cout << "Test 5: OK!" << std::endl;
}

int main() {
    try {
        std::cout << "=== Début des tests CGI ===" << std::endl;
        
        testPHPScript();
        testInvalidScript();
        testPOSTRequest();
        testEnvironmentVariables();
        testScriptErrors();
        
        std::cout << "=== Tous les tests ont réussi ! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Erreur pendant les tests : " << e.what() << std::endl;
        return 1;
    }
} 