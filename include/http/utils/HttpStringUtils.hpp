#ifndef HTTP_STRING_UTILS_HPP
#define HTTP_STRING_UTILS_HPP

#include <string>

namespace HttpStringUtils {

/**
 * @brief Normalise un ETag en supprimant les guillemets au début et à la fin
 * @param etag L'ETag à normaliser
 * @return L'ETag normalisé
 */
std::string normalizeETag(const std::string& etag);

} // namespace HttpStringUtils

#endif // HTTP_STRING_UTILS_HPP 