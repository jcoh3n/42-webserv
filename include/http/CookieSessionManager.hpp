#ifndef COOKIE_SESSION_MANAGER_HPP
#define COOKIE_SESSION_MANAGER_HPP

#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include <string>
#include <map>

// Définitions pour les cookies et sessions
#define SECRET_CODE_NAME "secret_code"
#define SECRET_CODE_MAX_AGE 60 // en secondes

class CookieSessionManager {
public:
    // Génère un nouvel ID de session (implémentation simple pour la démo)
    static std::string generateSessionId();

    // Définit le cookie de session dans la réponse
    static void setSessionCookie(HttpResponse& response, const std::string& session_id);

    // Vérifie si un cookie de session valide est présent dans la requête
    static bool hasValidSessionCookie(const HttpRequest& request);

    // Récupère l'ID de session à partir du cookie dans la requête
    static std::string getSessionId(const HttpRequest& request);

    // TODO: Ajouter une logique plus robuste pour la gestion des sessions (stockage, expiration, validation)

private:
    // Empêche l'instanciation de la classe utilitaire
    CookieSessionManager();
};

#endif // COOKIE_SESSION_MANAGER_HPP