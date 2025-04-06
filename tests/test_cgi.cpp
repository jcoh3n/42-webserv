#include <gtest/gtest.h>
#include "http/CGIHandler.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include <iostream>
#include <cassert>

class CGIHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Créer une requête HTTP de test
        request.setMethod("GET");
        request.setUri("/cgi-bin/cgi_debug.php");
        request.setQueryString("param1=value1&param2=value2");
        request.setHeader("Content-Type", "application/x-www-form-urlencoded");
    }

    HttpRequest request;
};

TEST_F(CGIHandlerTest, TestPHPScript) {
    // Créer une instance de CGIHandler
    CGIHandler handler(request, "./www/cgi-bin/cgi_debug.php", "/usr/bin/php-cgi");
    
    // Exécuter le script CGI
    HttpResponse response = handler.executeCGI();
    
    // Vérifier que la réponse est valide
    EXPECT_EQ(response.getStatusCode(), 200);
    EXPECT_EQ(response.getHeaders()["Content-Type"], "application/json");
    
    // Vérifier que le body contient les données attendues
    std::string body = response.getBody();
    EXPECT_NE(body.find("\"status\":\"success\""), std::string::npos);
    EXPECT_NE(body.find("\"method\":\"GET\""), std::string::npos);
    EXPECT_NE(body.find("\"query_string\":\"param1=value1&param2=value2\""), std::string::npos);
}

TEST_F(CGIHandlerTest, TestInvalidScript) {
    // Tester avec un script qui n'existe pas
    CGIHandler handler(request, "./www/cgi-bin/nonexistent.php", "/usr/bin/php-cgi");
    
    // Exécuter le script CGI
    HttpResponse response = handler.executeCGI();
    
    // Vérifier que la réponse est une erreur
    EXPECT_EQ(response.getStatusCode(), 404);
}

// Un test simple de la classe CGIHandler
int main() {
    HttpRequest request;
    request.setMethod("GET");
    // Mettre à jour l'URI pour utiliser cgi_debug.php au lieu de test.php
    request.setUri("/cgi-bin/cgi_debug.php");
    
    // Option 1: Utiliser un vrai interpréteur PHP
    // Assurez-vous d'avoir php-cgi installé sur votre système
    // Mettez à jour le chemin du script pour utiliser cgi_debug.php au lieu de test.php
    CGIHandler handler(request, "./www/cgi-bin/cgi_debug.php", "/usr/bin/php-cgi");
    
    // Option 2: Mock pour tester sans dépendance externe
    //CGIHandler handler(request, "./www/cgi-bin/test.php", "./mock_cgi");
    
    HttpResponse response = handler.executeCGI();
    
    std::cout << "Status: " << response.getStatus() << std::endl;
    std::cout << "Content-Type: " << response.getContentType() << std::endl;
    std::cout << "Body length: " << response.getBody().length() << std::endl;
    
    // Vérification simple que la requête a réussi
    assert(response.getStatus() == 200);
    assert(response.getContentType().find("application/json") != std::string::npos);
    assert(response.getBody().length() > 0);
    
    return 0;
} 