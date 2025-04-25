#ifndef COMMON_HPP
# define COMMON_HPP

// Includes standards
# include <iostream>
# include <string>
# include <vector>
# include <map>
# include <sstream>
# include <fstream>
# include <stdexcept>
# include <cstring>
# include <cstdlib>

// Includes système
# include <unistd.h>
# include <fcntl.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <poll.h>
# include <signal.h>
# include <errno.h>

// Définitions globales
# define MAX_CLIENTS 1024
# define BUFFER_SIZE 4096
# define DEFAULT_PORT 8080

// Couleurs ANSI pour le terminal
# define RESET   "\033[0m"
# define BLACK   "\033[30m"
# define RED     "\033[31m"
# define GREEN   "\033[32m"
# define YELLOW  "\033[33m"
# define BLUE    "\033[34m"
# define MAGENTA "\033[35m"
# define CYAN    "\033[36m"
# define WHITE   "\033[37m"
# define BOLD    "\033[1m"
# define DIM     "\033[2m"
# define UNDERLINE "\033[4m"

// Couleurs personnalisées pour les méthodes HTTP (correspondant aux couleurs du site)
# define COLOR_INFO  "\033[38;2;162;156;155m"      // #373F51 - couleur grise pour les logs d'info
# define COLOR_GET    "\033[38;2;0;186;188m"     // #00babc - couleur turquoise pour GET
# define COLOR_POST   "\033[38;2;147;112;219m"   // Lavender - moins intense, plus visible
# define COLOR_DELETE "\033[38;2;255;71;87m"     // #ff4757 - couleur rouge pour DELETE
# define COLOR_REDIRECT "\033[38;2;255;107;129m" // #ff6b81 - couleur pour les redirections
# define COLOR_ALIAS  "\033[38;2;165;94;234m"    // #a55eea - couleur pour les alias
# define COLOR_UPLOAD "\033[38;2;46;204;113m"    // #2ecc71 - couleur verte pour les uploads
# define COLOR_DOWNLOAD "\033[38;2;33;150;243m"  // #2196F3 - couleur bleue pour les downloads
# define COLOR_COOKIE "\033[38;2;160;82;45m" // Sienna - couleur marron pour les cookies
# define COOKIE_SESSION_NAME "session_id"

// Macros de logging (système) - simplifiées
# define LOG_INFO(x) std::cout << COLOR_INFO << "[INFO] " << x << RESET << std::endl
# define LOG_COOKIE(x) std::cout << COLOR_COOKIE << "[🍪 COOKIE] " << x << RESET << std::endl
# define LOG_SUCCESS(x) std::cout << GREEN << "[+] " << x << RESET << std::endl
# define LOG_WARNING(x) std::cerr << YELLOW << "[!] " << x << RESET << std::endl
# define LOG_ERROR(x)   std::cerr << RED << "[-] " << x << RESET << std::endl
# define LOG_DEBUG(x)   std::cerr << BLUE << "[D] " << x << RESET << std::endl  // Active les logs de debug
# define LOG_UPLOAD(x)  std::cout << COLOR_UPLOAD << "[↑] " << x << RESET << std::endl  // Upload en vert
# define LOG_REDIRECT(from, to, code) std::cout << COLOR_REDIRECT << "[↪] " << code << " • " << from << " → " << to << RESET << std::endl
# define LOG_ALIAS(from, to) std::cout << COLOR_ALIAS << "[⇄] " << from << " → " << to << RESET << std::endl
# define LOG_CGI_ERROR(x) // Désactivé pour éviter d'interrompre le flux des logs de requête/réponse

// Macros de logging (réseau et HTTP) - simplifiées
# define LOG_NETWORK(x) std::cout << BLUE << "→ " << x << RESET << std::endl
# define LOG_DOWNLOAD(x) std::cout << COLOR_DOWNLOAD << "[↓] " << x << RESET << std::endl  // Download en bleu
# define LOG_REQUEST(method, uri, code) \
    std::cout << \
    (std::string(method) == "GET" ? COLOR_GET : \
     std::string(method) == "POST" ? COLOR_POST : \
     std::string(method) == "DELETE" ? COLOR_DELETE : YELLOW) \
    << "→ " << method << RESET << " " << uri << RESET << std::endl; \
    std::cout << \
    (code >= 200 && code < 300 ? GREEN : \
     code >= 400 ? RED : \
     BLUE) \
    << "  ↳ " << code << RESET << std::endl

# define LOG_HTTP(x) std::cout << CYAN << "[HTTP] " << x << RESET << std::endl  // Active les logs HTTP détaillés

// Rétrocompatibilité
# define DEBUG(x) std::cerr << BLUE << "[DEBUG] " << x << RESET << std::endl  // Active les logs de debug

// Macros d'aide
# define UNUSED(x) (void)(x)

// Codes d'état HTTP courants
enum HTTPStatusCode {
    OK = 200,
    CREATED = 201,
    NO_CONTENT = 204,
    MOVED_PERMANENTLY = 301,
    BAD_REQUEST = 400,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    INTERNAL_SERVER_ERROR = 500
};

#endif 