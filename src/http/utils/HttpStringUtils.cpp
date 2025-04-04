#include "http/utils/HttpStringUtils.hpp"

namespace HttpStringUtils {

/**
 * @brief Normalise un ETag en supprimant les guillemets au début et à la fin
 * @param etag L'ETag à normaliser
 * @return L'ETag normalisé
 */
std::string normalizeETag(const std::string& etag) {
    std::string result = etag;
    // Supprimer les guillemets au début et à la fin s'ils existent
    if (!result.empty() && result[0] == '"') {
        result.erase(0, 1);
    }
    if (!result.empty() && result[result.size() - 1] == '"') {
        result.erase(result.size() - 1, 1);
    }
    return result;
}

} // namespace HttpStringUtils 