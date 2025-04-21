#include "../include/socket/Socket.hpp"

Socket::Socket() : fd(-1), is_non_blocking(false), is_closed(true) {}

Socket::Socket(int existing_fd) : fd(existing_fd), is_non_blocking(false), is_closed(false) {}

Socket::~Socket() {
    if (fd >= 0 && !is_closed) {
        this->close();
    }
}

void Socket::create() {
    fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        throw std::runtime_error("Socket creation failed: " + std::string(strerror(errno)));
    }
    is_closed = false;
}

void Socket::bind(int port) {
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (::bind(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Bind failed: " + std::string(strerror(errno)));
    }
}

void Socket::listen(int backlog) {
    if (::listen(fd, backlog) < 0) {
        throw std::runtime_error("Listen failed: " + std::string(strerror(errno)));
    }
}

Socket Socket::accept() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = ::accept(fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            throw std::runtime_error("Accept failed: " + std::string(strerror(errno)));
        }
        return Socket(-1);
    }
    
    return Socket(client_fd);
}

void Socket::setNonBlocking(bool non_blocking) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        throw std::runtime_error("Failed to get socket flags: " + std::string(strerror(errno)));
    }
    
    if (non_blocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    
    if (fcntl(fd, F_SETFL, flags) < 0) {
        throw std::runtime_error("Failed to set socket flags: " + std::string(strerror(errno)));
    }
    
    is_non_blocking = non_blocking;
}

void Socket::setReuseAddr(bool reuse) {
    int opt = reuse ? 1 : 0;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options: " + std::string(strerror(errno)));
    }
}

ssize_t Socket::send(const std::string& data) {
    return ::send(fd, data.c_str(), data.length(), 0);
}

ssize_t Socket::receive(char* buffer, size_t size) {
    return recv(fd, buffer, size, 0);
}

void Socket::close() {
    if (fd >= 0 && !is_closed) {
        ::close(fd);
        fd = -1;
        is_closed = true;
    }
}

bool Socket::isClosed() const {
    return is_closed;
}

int Socket::getFd() const {
    return fd;
}

bool Socket::isNonBlocking() const {
    return is_non_blocking;
} 