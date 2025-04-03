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

// Macros de logging (système)
# define LOG_INFO(x)    std::cout << CYAN << "[INFO] " << x << RESET << std::endl
# define LOG_SUCCESS(x) std::cout << GREEN << "[SUCCESS] " << x << RESET << std::endl
# define LOG_WARNING(x) std::cerr << YELLOW << "[WARN] " << x << RESET << std::endl
# define LOG_ERROR(x)   std::cerr << RED << "[ERROR] " << x << RESET << std::endl
# define LOG_DEBUG(x)   std::cerr << MAGENTA << "[DEBUG] " << x << RESET << std::endl

// Macros de logging (réseau et HTTP)
# define LOG_NETWORK(x) std::cout << BLUE << "⇄ " << x << RESET << std::endl
# define LOG_REQUEST(method, uri, code) std::cout << YELLOW << "➜ " << method << RESET << " " << uri << " " << DIM << "(" << code << ")" << RESET << std::endl
# define LOG_RESPONSE(code, bytes) \
    std::cout << (code >= 200 && code < 300 ? GREEN : (code >= 300 && code < 400 ? BLUE : RED)) \
              << "← HTTP " << code << RESET << " " << bytes << " bytes" << std::endl
# define LOG_HTTP(x) std::cout << CYAN << "► " << x << RESET << std::endl

// Rétrocompatibilité
# define DEBUG(x) LOG_DEBUG(x)

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