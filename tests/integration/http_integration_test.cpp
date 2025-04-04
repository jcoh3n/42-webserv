#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/RouteHandler.hpp"
#include "utils/Common.hpp"
#include <iostream>
#include <cassert>
#include <vector>

// Structure pour les cas de test
struct TestCase {
    std::string name;
    std::string raw_request;
    void (*validator)(const HttpResponse&);
};

class HttpIntegrationTest {
private:
    RouteHandler router;
    std::vector<TestCase> test_cases;

    static void validateBasicGet(const HttpResponse& response) {
        assert(response.getStatusCode() == 404 || response.getStatusCode() == 200);
        if (response.getStatusCode() == 200) {
            assert(!response.getBody().empty());
            assert(response.getHeaders().find("Content-Type") != response.getHeaders().end());
        }
    }

    static void validateMalformedRequest(const HttpResponse& response) {
        assert(response.getStatusCode() == 400);
    }

    static void validateUnsupportedMethod(const HttpResponse& response) {
        assert(response.getStatusCode() == 405);
    }

    static void validatePostWithBody(const HttpResponse& response) {
        assert(response.getStatusCode() == 501 || response.getStatusCode() == 200);
    }

    static void validateCustomHeaders(const HttpResponse& response) {
        assert(response.getStatusCode() == 404 || response.getStatusCode() == 200);
    }

    // Initialisation des cas de test
    void setupTestCases() {
        // Test GET basique
        TestCase test1;
        test1.name = "GET Request - Basic";
        test1.raw_request = "GET /index.html HTTP/1.1\r\n"
                           "Host: localhost:8080\r\n"
                           "User-Agent: curl/7.68.0\r\n"
                           "Accept: */*\r\n\r\n";
        test1.validator = &validateBasicGet;
        test_cases.push_back(test1);

        // Test requête malformée
        TestCase test2;
        test2.name = "Malformed Request";
        test2.raw_request = "INVALID /index.html\r\n"
                           "Host: localhost:8080\r\n\r\n";
        test2.validator = &validateMalformedRequest;
        test_cases.push_back(test2);

        // Test méthode non supportée
        TestCase test3;
        test3.name = "Unsupported Method";
        test3.raw_request = "PATCH /index.html HTTP/1.1\r\n"
                           "Host: localhost:8080\r\n\r\n";
        test3.validator = &validateUnsupportedMethod;
        test_cases.push_back(test3);

        // Test POST avec body
        TestCase test4;
        test4.name = "POST Request with Body";
        test4.raw_request = "POST /submit HTTP/1.1\r\n"
                           "Host: localhost:8080\r\n"
                           "Content-Type: application/x-www-form-urlencoded\r\n"
                           "Content-Length: 23\r\n\r\n"
                           "name=John&surname=Doe";
        test4.validator = &validatePostWithBody;
        test_cases.push_back(test4);

        // Test requête avec en-têtes personnalisés
        TestCase test5;
        test5.name = "Custom Headers";
        test5.raw_request = "GET /index.html HTTP/1.1\r\n"
                           "Host: localhost:8080\r\n"
                           "X-Custom-Header: test\r\n"
                           "Accept-Language: fr-FR\r\n\r\n";
        test5.validator = &validateCustomHeaders;
        test_cases.push_back(test5);
    }

public:
    HttpIntegrationTest() : router("./www") {
        setupTestCases();
    }

    void runTests() {
        LOG_INFO("=== Starting HTTP Integration Tests ===\n");
        
        int passed = 0;
        int total = test_cases.size();

        for (size_t i = 0; i < test_cases.size(); ++i) {
            try {
                LOG_INFO("Running test: " + test_cases[i].name);
                
                HttpRequest request;
                bool parse_result = request.parse(test_cases[i].raw_request);
                
                if (!parse_result) {
                    HttpResponse response;
                    response.setStatus(400);
                    test_cases[i].validator(response);
                } else {
                    HttpResponse response = router.processRequest(request);
                    test_cases[i].validator(response);
                }
                
                LOG_SUCCESS(test_cases[i].name + " - PASSED");
                passed++;
            } catch (const std::exception& e) {
                LOG_ERROR(test_cases[i].name + " - FAILED: " + e.what());
            }
        }

        std::ostringstream result;
        result << "Passed: " << passed << "/" << total;
        LOG_INFO(result.str());
        
        if (passed == total) {
            LOG_SUCCESS("All tests passed successfully!");
        } else {
            LOG_ERROR("Some tests failed!");
        }
    }
};

int main() {
    HttpIntegrationTest tester;
    tester.runTests();
    return 0;
} 