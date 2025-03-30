## ÉTAPE 1 - INITIALISATION DU SERVEUR (CORE)

### Objectif
Créer un serveur capable d'accepter des connexions entrantes sur un port spécifié.

### Détails techniques
1. **Création du socket**:
```cpp
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
if (server_fd < 0) {
    throw std::runtime_error("Socket creation failed: " + std::string(strerror(errno)));
}

Configuration du socket:

cppCopier// Option pour réutiliser l'adresse
int opt = 1;
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

// Configuration de l'adresse
struct sockaddr_in address;
memset(&address, 0, sizeof(address));
address.sin_family = AF_INET;
address.sin_addr.s_addr = INADDR_ANY;
address.sin_port = htons(port);

Binding et écoute:

cppCopierif (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    close(server_fd);
    throw std::runtime_error("Bind failed: " + std::string(strerror(errno)));
}
if (listen(server_fd, SOMAXCONN) < 0) {
    close(server_fd);
    throw std::runtime_error("Listen failed: " + std::string(strerror(errno)));
}

Configuration non-bloquante:

cppCopierif (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0) {
    close(server_fd);
    throw std::runtime_error("Failed to set non-blocking mode: " + std::string(strerror(errno)));
}

Gestion des signaux:

cppCopiervoid setupSignalHandlers() {
    struct sigaction sa;
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, NULL);  // Ctrl+C
    sigaction(SIGTERM, &sa, NULL); // Signal de terminaison
}

void handleSignal(int signal) {
    // Journaliser l'événement
    std::cout << "Received signal " << signal << ", shutting down gracefully..." << std::endl;
    
    // Nettoyer les ressources et fermer les sockets
    cleanupAndExit();
}

void cleanupAndExit() {
    // Fermer le socket principal
    if (server_fd >= 0) {
        close(server_fd);
    }
    
    // Fermer tous les sockets clients
    for (int i = 0; i < nfds; i++) {
        if (fds[i].fd >= 0) {
            close(fds[i].fd);
        }
    }
    
    // Libérer les autres ressources
    // ...
    
    exit(0);
}
Points de validation

 Le serveur se lance sans erreur
 Le port spécifié est bien occupé (vérifiable avec netstat -tulnp)
 Le serveur reste stable (pas de crash spontané)
 Accepte les connexions via telnet localhost <port>
 Gère correctement les signaux d'arrêt (SIGINT, SIGTERM)

Tests recommandés

Test de démarrage de base:

bashCopier$ ./webserv config.conf

Test de connexion:

bashCopier$ telnet localhost 8080
Trying 127.0.0.1...
Connected to localhost.

Test avec port occupé:

bashCopier# Démarrer un autre service sur le port 8080
$ python -m http.server 8080
# Puis essayer de démarrer webserv sur le même port
$ ./webserv config.conf
# Vérifier la gestion d'erreur appropriée

Test d'arrêt gracieux:

bashCopier# Démarrer le serveur
$ ./webserv config.conf
# Dans un autre terminal, envoyer un signal SIGINT
$ pkill -INT webserv
# Vérifier que le serveur s'arrête proprement
Bonnes pratiques

Libérer toutes les ressources lors de la fermeture
Journaliser les erreurs de manière détaillée
Utiliser des messages d'erreur explicites
Vérifier les valeurs de retour de toutes les fonctions système