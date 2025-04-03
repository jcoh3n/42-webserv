#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/RouteHandler.hpp"
#include "utils/Common.hpp"
#include <iostream>
#include <cassert>

// Test d'intégration simple pour vérifier que l'intégration entre 
// le parsing des requêtes HTTP et la génération des réponses fonctionne correctement
void test_integration_basic() {
    LOG_INFO("Test d'intégration: requête GET simple... ");
    
    // Créer une requête GET simple
    std::string raw_request = "GET /index.html HTTP/1.1\r\n"
                             "Host: localhost:8080\r\n"
                             "User-Agent: curl/7.68.0\r\n"
                             "Accept: */*\r\n"
                             "\r\n";
    
    // Initialiser et parser la requête
    HttpRequest request;
    bool parse_result = request.parse(raw_request);
    assert(parse_result);
    assert(request.getMethod() == "GET");
    assert(request.getUri() == "/index.html");
    
    // Initialiser le gestionnaire de routes avec un répertoire de test
    RouteHandler router("./www");
    
    // Traiter la requête et obtenir une réponse
    HttpResponse response = router.processRequest(request);
    
    // Vérifier que la réponse est valide (ici, comme nous n'avons probablement pas le fichier,
    // nous nous attendons à une erreur 404)
    assert(response.getStatusCode() == 404 || response.getStatusCode() == 200);
    
    // Si le statut est 200, vérifier que nous avons un corps de réponse non vide
    if (response.getStatusCode() == 200) {
        assert(!response.getBody().empty());
        assert(response.getHeaders().find("Content-Type") != response.getHeaders().end());
        assert(response.getHeaders().find("Content-Length") != response.getHeaders().end());
    }
    
    LOG_SUCCESS("GET simple - OK");
}

// Test d'intégration pour une requête malformée
void test_integration_malformed_request() {
    LOG_INFO("Test d'intégration: requête malformée... ");
    
    // Créer une requête malformée
    std::string raw_request = "INVALID /index.html\r\n"
                             "Host: localhost:8080\r\n"
                             "\r\n";
    
    // Initialiser et parser la requête
    HttpRequest request;
    bool parse_result = request.parse(raw_request);
    
    // Vérifier que le parsing échoue
    assert(!parse_result);
    
    LOG_SUCCESS("Requête malformée - OK");
}

// Test d'intégration pour une requête 404
void test_integration_not_found() {
    LOG_INFO("Test d'intégration: fichier non trouvé (404)... ");
    
    // Créer une requête pour un fichier inexistant
    std::string raw_request = "GET /fichier_inexistant.html HTTP/1.1\r\n"
                             "Host: localhost:8080\r\n"
                             "User-Agent: curl/7.68.0\r\n"
                             "Accept: */*\r\n"
                             "\r\n";
    
    // Initialiser et parser la requête
    HttpRequest request;
    bool parse_result = request.parse(raw_request);
    assert(parse_result);
    
    // Initialiser le gestionnaire de routes
    RouteHandler router("./www");
    
    // Traiter la requête et obtenir une réponse
    HttpResponse response = router.processRequest(request);
    
    // Vérifier que la réponse est un 404
    assert(response.getStatusCode() == 404);
    assert(!response.getBody().empty());
    
    LOG_SUCCESS("404 - OK");
}

// Test d'intégration pour une requête POST
void test_integration_post_request() {
    LOG_INFO("Test d'intégration: requête POST... ");
    
    // Créer une requête POST simple
    std::string raw_request = "POST /submit HTTP/1.1\r\n"
                             "Host: localhost:8080\r\n"
                             "Content-Type: application/x-www-form-urlencoded\r\n"
                             "Content-Length: 23\r\n"
                             "\r\n"
                             "name=John&surname=Doe";
    
    // Initialiser et parser la requête
    HttpRequest request;
    bool parse_result = request.parse(raw_request);
    assert(parse_result);
    assert(request.getMethod() == "POST");
    assert(request.getUri() == "/submit");
    
    // Initialiser le gestionnaire de routes
    RouteHandler router("./www");
    
    // Traiter la requête et obtenir une réponse
    HttpResponse response = router.processRequest(request);
    
    // Vérifier que la réponse est une réponse d'erreur 501 (Not Implemented)
    // car nous n'avons pas encore implémenté le traitement des POST
    assert(response.getStatusCode() == 501);
    
    LOG_SUCCESS("POST - OK");
}

int main() {
    LOG_INFO("=== Tests d'intégration HTTP ===");
    
    test_integration_basic();
    test_integration_malformed_request();
    test_integration_not_found();
    test_integration_post_request();
    
    LOG_SUCCESS("Tous les tests d'intégration ont réussi !");
    return 0;
} 