#ifndef SOCKET_HPP
# define SOCKET_HPP

# include "../utils/Common.hpp"

class Socket {
private:
    int fd;
    bool is_non_blocking;

public:
    Socket();
    Socket(int existing_fd);
    ~Socket();

    // Méthodes de création
    void create();
    void bind(int port);
    void listen(int backlog = SOMAXCONN);
    Socket accept();

    // Configuration
    void setNonBlocking(bool non_blocking);
    void setReuseAddr(bool reuse);

    // Opérations d'E/S
    ssize_t send(const std::string& data);
    ssize_t receive(char* buffer, size_t size);

    // Opérations de fermeture
    void close();

    // Accesseurs
    int getFd() const;
    bool isNonBlocking() const;
};

#endif 