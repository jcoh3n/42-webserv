#include "http/CookieSessionManager.hpp"
#include "utils/Common.hpp" // Pour LOG_INFO
#include <sstream>
#include <cstdlib> // Pour rand() et srand()
#include <ctime>   // Pour time()

// Initialisation du générateur de nombres aléatoires pour generateSessionId
namespace {
    bool seeded = false;
}

std::string CookieSessionManager::generateSessionId() {
    if (!seeded) {
        srand(static_cast<unsigned int>(time(0)));
        seeded = true;
    }
    // Implémentation simple pour la démo: un nombre aléatoire
    std::stringstream ss;
    ss << "messi>ronaldo_" << rand();
    return ss.str();
}

void CookieSessionManager::setSessionCookie(HttpResponse& response, const std::string& session_id) {
    std::stringstream ss;
    ss << SECRET_CODE_MAX_AGE;
    response.setHeader("Set-Cookie", std::string(SECRET_CODE_NAME) + "=" + session_id + "; Path=/restricted-area; Max-Age=" + ss.str() + "; HttpOnly");
    LOG_COOKIE("Cookie défini → " << SECRET_CODE_NAME << "=" << session_id);
}

bool CookieSessionManager::hasValidSessionCookie(const HttpRequest& request) {
    std::string session_cookie = request.getHeader("Cookie");
    // Pour cette demo, on considere que la presence du cookie suffit
    // Dans une vraie application, il faudrait valider l'ID de session (e.g., dans une map de sessions actives)
    return session_cookie.find(std::string(SECRET_CODE_NAME) + "=") != std::string::npos;
}

std::string CookieSessionManager::getSessionId(const HttpRequest& request) {
    std::string session_cookie = request.getHeader("Cookie");
    std::string session_id = "";
    std::string search_str = std::string(SECRET_CODE_NAME) + "=";
    size_t start_pos = session_cookie.find(search_str);
    if (start_pos != std::string::npos) {
        start_pos += search_str.length();
        size_t end_pos = session_cookie.find(";", start_pos);
        if (end_pos == std::string::npos) {
            session_id = session_cookie.substr(start_pos);
        } else {
            session_id = session_cookie.substr(start_pos, end_pos - start_pos);
        }
    }
    return session_id;
}

// Empêche l'instanciation de la classe utilitaire
CookieSessionManager::CookieSessionManager() {}