#ifndef SOCKET_HPP
# define SOCKET_HPP

# include "../utils/Common.hpp"

/**
 * @brief Classe encapsulant un socket TCP
 * 
 * Cette classe fournit une interface orientée objet pour les opérations de socket,
 * avec une gestion appropriée des ressources et des erreurs.
 */
class Socket {
private:
    int fd;              // Descripteur de fichier du socket
    bool is_non_blocking;// État du mode non-bloquant
    bool is_closed;      // État de fermeture du socket

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
    bool isClosed() const;

    // Getters
    int getFd() const;
    bool isNonBlocking() const;
};

#endif 