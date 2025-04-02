#include "http/ResponseHandler.hpp"
#include <sys/socket.h>
#include <fstream>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <sstream>

/**
 * @brief Convertit une valeur numérique en chaîne de caractères
 * @tparam T Type de la valeur numérique
 * @param value La valeur à convertir
 * @return La chaîne de caractères correspondante
 */
template<typename T>
static std::string numberToString(T value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// Initialisation de la variable statique
std::map<int, std::string> ResponseHandler::pending_responses;

/**
 * @brief Constructeur par défaut
 */
ResponseHandler::ResponseHandler() {
}

/**
 * @brief Destructeur
 */
ResponseHandler::~ResponseHandler() {
}

/**
 * @brief Envoie une réponse HTTP complète au client
 * @param client_fd Le descripteur de fichier du client
 * @param response La réponse HTTP à envoyer
 * @param request La requête HTTP originale
 * @return Le nombre d'octets envoyés, ou -1 en cas d'erreur
 * 
 * Cette fonction envoie la réponse HTTP au client en tenant compte
 * du type de requête (HEAD ne renvoie que les en-têtes) et gère
 * les cas où le socket n'est pas prêt pour l'envoi complet.
 */
ssize_t ResponseHandler::sendResponse(int client_fd, const HttpResponse& response, const HttpRequest& request) {
    std::string raw_response;
    
    // Pour les requêtes HEAD, n'envoyer que les headers
    if (request.getMethod() == "HEAD") {
        raw_response = response.buildHeadResponse();
    } else {
        raw_response = response.build();
    }
    
    size_t total_sent = 0;
    size_t to_send = raw_response.size();
    ssize_t last_sent = 0;
    
    while (total_sent < to_send) {
        last_sent = send(client_fd, raw_response.c_str() + total_sent, to_send - total_sent, 0);
        if (last_sent <= 0) {
            if (last_sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // Socket non prêt, stocker le reste pour l'envoyer plus tard
                storePendingResponse(client_fd, raw_response.substr(total_sent));
                return total_sent;
            } else {
                // Erreur réelle ou connexion fermée
                return last_sent;
            }
        }
        total_sent += last_sent;
    }
    
    return total_sent;
}

/**
 * @brief Envoie un fichier de grande taille au client en mode chunked
 * @param client_fd Le descripteur de fichier du client
 * @param file_path Le chemin du fichier à envoyer
 * @param request La requête HTTP originale
 * 
 * Cette fonction optimise l'envoi de fichiers volumineux en utilisant
 * le transfert chunked pour les fichiers > 1Mo. Pour les requêtes HEAD,
 * seuls les en-têtes sont envoyés.
 */
void ResponseHandler::sendLargeFile(int client_fd, const std::string& file_path, const HttpRequest& request) {
    // Ouvrir le fichier
    std::ifstream file(file_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        HttpResponse error_response = createErrorResponse(404);
        sendResponse(client_fd, error_response, request);
        return;
    }
    
    // Préparer l'en-tête de la réponse
    HttpResponse response;
    response.setStatus(200);
    response.setHeader("Content-Type", getMimeType(file_path));
    
    // Pour HEAD, on n'envoie que les headers
    if (request.getMethod() == "HEAD") {
        // Obtenir la taille du fichier pour Content-Length
        file.seekg(0, std::ios::end);
        std::streamsize file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::ostringstream size_str;
        size_str << file_size;
        response.setHeader("Content-Length", size_str.str());
        sendResponse(client_fd, response, request);
        return;
    }

    // Pour les fichiers volumineux, utiliser le transfert chunked
    if (file.seekg(0, std::ios::end).tellg() > 1024 * 1024) { // Plus de 1 Mo
        file.seekg(0, std::ios::beg);
        response.setChunkedTransfer();
        
        // Envoyer les headers
        std::string headers = response.buildHeadResponse();
        send(client_fd, headers.c_str(), headers.size(), 0);
        
        // Lire et envoyer le fichier par morceaux
        const size_t CHUNK_SIZE = 8192; // 8KB chunks
        char buffer[CHUNK_SIZE];
        char chunk_header[20]; // Pour stocker l'en-tête de chunk (taille en hex)
        
        while (file.good() && !file.eof()) {
            file.read(buffer, CHUNK_SIZE);
            std::streamsize bytes_read = file.gcount();
            
            if (bytes_read > 0) {
                // Formater l'en-tête du chunk (taille en hexadécimal)
                snprintf(chunk_header, sizeof(chunk_header), "%zx\r\n", static_cast<size_t>(bytes_read));
                
                // Envoyer l'en-tête du chunk
                send(client_fd, chunk_header, strlen(chunk_header), 0);
                
                // Envoyer les données du chunk
                send(client_fd, buffer, bytes_read, 0);
                
                // Envoyer le terminateur de chunk
                send(client_fd, "\r\n", 2, 0);
            }
        }
        
        // Envoyer le chunk final (zéro)
        send(client_fd, "0\r\n\r\n", 5, 0);
    } else {
        // Pour les fichiers plus petits, lire tout en mémoire et envoyer normalement
        file.seekg(0, std::ios::beg);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        response.setBody(content, getMimeType(file_path));
        sendResponse(client_fd, response, request);
    }
}

/**
 * @brief Stocke une réponse partielle pour l'envoyer plus tard
 * @param client_fd Le descripteur de fichier du client
 * @param remaining_data Les données restantes à envoyer
 * 
 * Utilisé lorsqu'un socket n'est pas prêt à recevoir toutes les données.
 * Les données restantes sont stockées pour être envoyées ultérieurement.
 */
void ResponseHandler::storePendingResponse(int client_fd, const std::string& remaining_data) {
    pending_responses[client_fd] = remaining_data;
}

/**
 * @brief Vérifie si un client a une réponse en attente
 * @param client_fd Le descripteur de fichier du client
 * @return true si le client a des données en attente, false sinon
 */
bool ResponseHandler::hasPendingResponse(int client_fd) {
    return pending_responses.find(client_fd) != pending_responses.end();
}

/**
 * @brief Continue l'envoi d'une réponse partielle
 * @param client_fd Le descripteur de fichier du client
 * @return true si l'envoi est terminé ou en cas d'erreur, false si l'envoi est toujours en cours
 * 
 * Tente de terminer l'envoi d'une réponse partiellement envoyée.
 * Retourne true quand tout a été envoyé ou en cas d'erreur fatale.
 */
bool ResponseHandler::continueSendingPendingResponse(int client_fd) {
    if (!hasPendingResponse(client_fd)) {
        return true;
    }
    
    std::string& remaining = pending_responses[client_fd];
    ssize_t sent = send(client_fd, remaining.c_str(), remaining.size(), 0);
    
    if (sent <= 0) {
        if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Toujours pas prêt, réessayer plus tard
            return false;
        } else {
            // Erreur réelle ou connexion fermée
            clearPendingResponse(client_fd);
            return false;
        }
    } else if (static_cast<size_t>(sent) < remaining.size()) {
        // Mise à jour de ce qui reste à envoyer
        remaining = remaining.substr(sent);
        return false;
    } else {
        // Tout a été envoyé
        clearPendingResponse(client_fd);
        return true;
    }
}

/**
 * @brief Supprime une réponse en attente
 * @param client_fd Le descripteur de fichier du client
 * 
 * Nettoie les données en attente pour un client spécifique,
 * typiquement après un envoi réussi ou une erreur fatale.
 */
void ResponseHandler::clearPendingResponse(int client_fd) {
    pending_responses.erase(client_fd);
} 