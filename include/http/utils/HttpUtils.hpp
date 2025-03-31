#ifndef HTTP_UTILS_HPP
#define HTTP_UTILS_HPP

#include <string>
#include <sstream>

/**
 * @brief Classe d'utilitaires pour HTTP
 */
class HttpUtils {
public:
    /**
     * @brief Décode une chaîne URL-encodée
     * 
     * @param encoded Chaîne encodée (ex: "hello%20world")
     * @return Chaîne décodée (ex: "hello world")
     */
    static std::string urlDecode(const std::string& encoded);
    
    /**
     * @brief Vérifie si l'URI est sécurisée (pas de traversée de répertoire)
     * 
     * @param uri URI à vérifier
     * @return true si l'URI est sécurisée
     */
    static bool isUriSafe(const std::string& uri);
    
    /**
     * @brief Vérifie si la méthode HTTP est valide
     * 
     * @param method Méthode à vérifier
     * @return true si la méthode est supportée
     */
    static bool isMethodSupported(const std::string& method);
    
    /**
     * @brief Vérifie si la version HTTP est supportée
     * 
     * @param version Version à vérifier
     * @return true si la version est supportée
     */
    static bool isVersionSupported(const std::string& version);
    
    /**
     * @brief Valide la taille du contenu
     * 
     * @param content_length Taille du contenu
     * @param max_size Taille maximale autorisée
     * @return true si la taille est valide
     */
    static bool isContentLengthValid(size_t content_length, size_t max_size);

private:
    // Constantes
    static const std::string ALLOWED_METHODS[];
    static const size_t METHOD_COUNT;
    
    // Constructeur privé pour empêcher l'instanciation
    HttpUtils() {}
};

#endif // HTTP_UTILS_HPP 