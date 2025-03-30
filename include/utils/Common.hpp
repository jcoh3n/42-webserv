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

// Macros d'aide
# define UNUSED(x) (void)(x)
# define DEBUG(x) std::cerr << "[DEBUG] " << x << std::endl

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