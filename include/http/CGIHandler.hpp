#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include <string>
#include <vector>
#include <map>

// Définition du timeout CGI en secondes
#define CGI_TIMEOUT 3

class CGIHandler {
private:
    const HttpRequest& request_;
    std::string script_path_;
    std::string interpreter_;
    std::string root_directory_;
    std::map<int, std::string> error_pages_;
    
    // Méthodes privées
    bool executeCGIScript(int pipe_in[2], int pipe_out[2]);
    std::vector<std::string> prepareEnvironment() const;
    HttpResponse parseCGIOutput(const std::string& output);
    bool isCGIScript() const;
    
    // Méthode pour servir une page d'erreur personnalisée
    HttpResponse serveErrorPage(int error_code, const std::string& message);
    
public:
    CGIHandler(const HttpRequest& req, const std::string& script_path, const std::string& interpreter);
    CGIHandler(const HttpRequest& req, const std::string& script_path, const std::string& interpreter, 
              const std::string& root_dir, const std::map<int, std::string>& error_pages);
    ~CGIHandler();
    
    // Méthode principale pour exécuter le script CGI
    HttpResponse executeCGI();
};

#endif // CGI_HANDLER_HPP 