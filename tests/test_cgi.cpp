#include <gtest/gtest.h>
#include "http/CGIHandler.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

class CGIHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Créer une requête HTTP de test
        request.setMethod("GET");
        request.setUri("/cgi-bin/test.php");
        request.setQueryString("param1=value1&param2=value2");
        request.setHeader("Content-Type", "application/x-www-form-urlencoded");
    }

    HttpRequest request;
};

TEST_F(CGIHandlerTest, TestPHPScript) {
    // Créer une instance de CGIHandler
    CGIHandler handler(request, "./www/cgi-bin/test.php", "/usr/bin/php-cgi");
    
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

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 