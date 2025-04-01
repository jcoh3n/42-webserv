#ifndef RESPONSE_HANDLER_HPP
#define RESPONSE_HANDLER_HPP

#include "http/HttpResponse.hpp"
#include "http/HttpRequest.hpp"
#include <string>
#include <map>
#include <sys/types.h>

/**
 * @brief Classe pour gérer l'envoi des réponses HTTP aux clients
 */
class ResponseHandler {
public:
    // Constructeur et destructeur
    ResponseHandler();
    ~ResponseHandler();

    // Méthodes principales pour l'envoi de réponses
    static ssize_t sendResponse(int client_fd, const HttpResponse& response, const HttpRequest& request);
    static void sendLargeFile(int client_fd, const std::string& file_path, const HttpRequest& request);
    
    // Méthodes pour gérer les réponses partielles (quand socket non prêt)
    static void storePendingResponse(int client_fd, const std::string& remaining_data);
    static bool hasPendingResponse(int client_fd);
    static bool continueSendingPendingResponse(int client_fd);
    static void clearPendingResponse(int client_fd);

private:
    // Stockage pour les réponses partielles à reprendre
    static std::map<int, std::string> pending_responses;
};

#endif // RESPONSE_HANDLER_HPP 