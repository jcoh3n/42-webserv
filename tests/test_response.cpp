#include "http/HttpResponse.hpp"
#include "http/HttpRequest.hpp"
#include "http/ResponseHandler.hpp"
#include "http/utils/FileUtils.hpp"
#include "utils/Common.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

// Les fonctions de log utilisent maintenant les macros définies dans Common.hpp
void log_success(const std::string& message) {
    LOG_SUCCESS(message);
}

void log_error(const std::string& message) {
    LOG_ERROR(message);
}

void test_http_response_basic() {
    HttpResponse response;
    
    // Test des valeurs par défaut
    assert(response.getStatusCode() == 200);
    assert(response.getStatusMessage() == "OK");
    
    // Test des headers par défaut
    const std::map<std::string, std::string>& headers = response.getHeaders();
    assert(headers.find("Server") != headers.end());
    assert(headers.find("Connection") != headers.end());
    
    log_success("Création basique de HttpResponse");
}

void test_http_response_status() {
    HttpResponse response;
    
    // Test différents codes de statut
    response.setStatus(404);
    assert(response.getStatusCode() == 404);
    assert(response.getStatusMessage() == "Not Found");
    
    response.setStatus(500, "Custom Error");
    assert(response.getStatusCode() == 500);
    assert(response.getStatusMessage() == "Custom Error");
    
    log_success("Changement de statut HttpResponse");
}

void test_http_response_body() {
    HttpResponse response;
    
    // Test ajout de body
    response.setBody("Hello World", "text/plain");
    assert(response.getBody() == "Hello World");
    
    const std::map<std::string, std::string>& headers = response.getHeaders();
    assert(headers.find("Content-Type") != headers.end());
    assert(headers.find("Content-Length") != headers.end());
    assert(headers.at("Content-Type") == "text/plain");
    assert(headers.at("Content-Length") == "11"); // "Hello World" = 11 caractères
    
    log_success("Ajout de body à HttpResponse");
}

void test_http_response_build() {
    HttpResponse response;
    response.setStatus(200);
    response.setBody("Test Body", "text/plain");
    
    std::string raw_response = response.build();
    
    // Vérifier la présence des éléments clés
    assert(raw_response.find("HTTP/1.1 200 OK") == 0);
    assert(raw_response.find("Content-Type: text/plain") != std::string::npos);
    assert(raw_response.find("Content-Length: 9") != std::string::npos);
    assert(raw_response.find("\r\n\r\nTest Body") != std::string::npos);
    
    log_success("Construction de chaîne de réponse HTTP");
}

void test_error_response() {
    HttpResponse error = createErrorResponse(404);
    
    assert(error.getStatusCode() == 404);
    assert(error.getStatusMessage() == "Not Found");
    assert(!error.getBody().empty());
    assert(error.getBody().find("404") != std::string::npos);
    
    log_success("Création de réponse d'erreur");
}

void test_mime_type() {
    assert(getMimeType("test.html") == "text/html");
    assert(getMimeType("test.jpg") == "image/jpeg");
    assert(getMimeType("test.unknown") == "application/octet-stream");
    
    log_success("Détection de types MIME");
}

void test_redirect() {
    HttpResponse redirect;
    redirect.setRedirect("/new-location", 301);
    
    assert(redirect.getStatusCode() == 301);
    assert(redirect.getHeaders().find("Location") != redirect.getHeaders().end());
    assert(redirect.getHeaders().at("Location") == "/new-location");
    assert(redirect.getBody().find("301") != std::string::npos);
    assert(redirect.getBody().find("Moved Permanently") != std::string::npos);
    
    log_success("Réponses de redirection");
}

void test_not_modified() {
    HttpResponse response;
    std::string etag = "\"123456\"";
    response.setNotModified(etag);
    
    assert(response.getStatusCode() == 304);
    assert(response.getHeaders().find("ETag") != response.getHeaders().end());
    assert(response.getHeaders().at("ETag") == etag);
    assert(response.getBody().empty());
    
    log_success("Réponses 304 Not Modified");
}

int main() {
    LOG_INFO("=== TEST DU SYSTÈME DE RÉPONSE HTTP ===");
    
    test_http_response_basic();
    test_http_response_status();
    test_http_response_body();
    test_http_response_build();
    test_error_response();
    test_mime_type();
    test_redirect();
    test_not_modified();
    
    LOG_SUCCESS("Tous les tests ont réussi !");
    return 0;
} 