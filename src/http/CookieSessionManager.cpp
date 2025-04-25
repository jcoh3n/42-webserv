#include "http/CookieSessionManager.hpp"
#include "utils/Common.hpp" // Pour LOG_INFO
#include <sstream>

// Initialisation du générateur de nombres aléatoires pour generateSessionId
namespace {
    // Variables statiques pour simuler la gestion de session côté serveur
    time_t session_creation_time = 0;
    std::string active_session_id = "";
}

std::string CookieSessionManager::generateSessionId() {
    std::stringstream ss;
    ss << SECRET_CODE_VALUE;
    return ss.str();
}

void CookieSessionManager::setSessionCookie(HttpResponse& response, const std::string& session_id) {
    // Enregistrer l'ID de session et l'heure de création côté serveur (simulation)
    active_session_id = session_id;
    session_creation_time = time(0);

    std::stringstream ss;
    ss << SECRET_CODE_MAX_AGE;
    // Utiliser Max-Age pour indiquer au navigateur quand expirer le cookie
    response.setHeader("Set-Cookie", std::string(SECRET_CODE_NAME) + "=" + session_id + "; Path=/restricted-area; Max-Age=" + ss.str());
    LOG_COOKIE("Cookie défini → " << SECRET_CODE_NAME << "=" << session_id << " | SESSION_TIME=" << SECRET_CODE_MAX_AGE << "s");
}

bool CookieSessionManager::hasValidSessionCookie(const HttpRequest& request) {
    std::string session_cookie_header = request.getHeader("Cookie");
    std::string search_str = std::string(SECRET_CODE_NAME) + "=";
    size_t start_pos = session_cookie_header.find(search_str);

    if (start_pos == std::string::npos) {
        LOG_COOKIE("Validation session: Cookie absent.");
        return false; // Cookie non trouvé dans l'en-tête
    }

    // Extraire l'ID de session du cookie reçu
    std::string received_session_id = "";
    start_pos += search_str.length();
    size_t end_pos = session_cookie_header.find(";", start_pos);
    if (end_pos == std::string::npos) {
        received_session_id = session_cookie_header.substr(start_pos);
    } else {
        received_session_id = session_cookie_header.substr(start_pos, end_pos - start_pos);
    }

    // Vérifier si l'ID de session reçu correspond à l'ID de session actif côté serveur
    if (received_session_id != active_session_id || active_session_id.empty()) {
        LOG_COOKIE("Validation session: ID de session reçu (" << received_session_id << ") ne correspond pas à l'ID actif côté serveur (" << active_session_id << ").");
        return false; // ID de session ne correspond pas ou aucune session active côté serveur
    }

    // Vérifier l'expiration côté serveur
    time_t now = time(0);
    if (difftime(now, session_creation_time) > SECRET_CODE_MAX_AGE) {
        LOG_COOKIE("Validation session: Session expirée côté serveur. Temps écoulé: " << difftime(now, session_creation_time) << "s");
        active_session_id = ""; // Invalider la session côté serveur
        session_creation_time = 0;
        return false; // Session expirée
    }

    LOG_COOKIE("Validation session: Cookie trouvé et session valide côté serveur.");
    return true; // Cookie présent et session valide côté serveur
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