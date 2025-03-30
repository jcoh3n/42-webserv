## ÉTAPE 2 - BOUCLE ÉVÉNEMENTIELLE AVEC POLL()

### Objectif
Mettre en place une boucle principale capable de gérer plusieurs connexions simultanées de manière non-bloquante avec une gestion efficace des timeouts et des erreurs.

### Détails techniques

1. **Initialisation de poll()**:
```cpp
struct pollfd fds[MAX_CLIENTS + 1];
memset(fds, 0, sizeof(fds));

// Configuration du socket serveur
fds[0].fd = server_fd;
fds[0].events = POLLIN;  // Surveille les entrées
nfds = 1;  // Nombre de FDs surveillés

Boucle principale avec timeout:

cppCopier// Structure pour suivre l'activité des clients
struct ClientActivity {
    time_t last_activity;
    bool has_pending_data;
};

std::map<int, ClientActivity> client_activity;
const int CONNECTION_TIMEOUT = 60; // 60 secondes

while (running) {
    // Calculer le timeout minimum pour poll()
    int timeout = calculateNextTimeout(client_activity);
    
    int ready = poll(fds, nfds, timeout);
    if (ready == -1) {
        if (errno == EINTR) {
            // Interruption par signal, continuer
            continue;
        }
        // Gestion erreur fatale
        throw std::runtime_error("Poll failed: " + std::string(strerror(errno)));
    }
    
    // Vérifier les timeouts
    checkClientTimeouts(fds, client_activity);
    
    // Vérifier les nouveaux clients
    if (fds[0].revents & POLLIN) {
        acceptNewClient(fds, &nfds, server_fd, client_activity);
        ready--;
    }
    
    // Vérifier les clients existants
    if (ready > 0) {
        checkExistingClients(fds, nfds, client_activity);
    }
}

Calcul du timeout optimal:

cppCopierint calculateNextTimeout(const std::map<int, ClientActivity>& client_activity) {
    time_t now = time(NULL);
    int min_timeout = CONNECTION_TIMEOUT * 1000; // Conversion en millisecondes
    
    for (const auto& client : client_activity) {
        int elapsed = now - client.second.last_activity;
        if (elapsed < CONNECTION_TIMEOUT) {
            int remaining = (CONNECTION_TIMEOUT - elapsed) * 1000;
            min_timeout = std::min(min_timeout, remaining);
        }
    }
    
    return min_timeout > 0 ? min_timeout : 100; // Minimum 100ms pour éviter 100% CPU
}

Vérification des timeouts clients:

cppCopiervoid checkClientTimeouts(struct pollfd fds[], std::map<int, ClientActivity>& client_activity) {
    time_t now = time(NULL);
    std::vector<int> timedout_clients;
    
    for (auto& client : client_activity) {
        if (now - client.second.last_activity > CONNECTION_TIMEOUT) {
            timedout_clients.push_back(client.first);
        }
    }
    
    for (int client_fd : timedout_clients) {
        std::cout << "Client " << client_fd << " timed out." << std::endl;
        
        // Envoyer une réponse 408 Request Timeout si nécessaire
        if (client_activity[client_fd].has_pending_data) {
            HttpResponse timeout_response;
            timeout_response.setStatus(408);
            timeout_response.setBody("Request Timeout", "text/plain");
            sendResponse(client_fd, timeout_response);
        }
        
        close(client_fd);
        client_activity.erase(client_fd);
        removeClient(fds, &nfds, client_fd);
    }
}

Acceptation des nouveaux clients:

cppCopiervoid acceptNewClient(struct pollfd fds[], int *nfds, int server_fd, std::map<int, ClientActivity>& client_activity) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (true) { // Accepter tous les clients en attente
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Plus de connexions en attente
                break;
            } else {
                // Erreur réelle - journaliser et continuer
                std::cerr << "Accept error: " << strerror(errno) << std::endl;
                break;
            }
        }
        
        // Limiter le nombre de clients
        if (*nfds >= MAX_CLIENTS + 1) {
            // Refuser la connexion avec 503 Service Unavailable
            HttpResponse busy_response;
            busy_response.setStatus(503);
            busy_response.setBody("Server Too Busy", "text/plain");
            sendResponse(client_fd, busy_response);
            close(client_fd);
            continue;
        }
        
        // Configurer en mode non-bloquant
        if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
            close(client_fd);
            continue;
        }
        
        // Ajouter à poll
        fds[*nfds].fd = client_fd;
        fds[*nfds].events = POLLIN; // On commence par lire la requête
        (*nfds)++;
        
        // Initialiser l'activité du client
        ClientActivity activity;
        activity.last_activity = time(NULL);
        activity.has_pending_data = false;
        client_activity[client_fd] = activity;
        
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), ip, INET_ADDRSTRLEN);
        std::cout << "New client connected: " << ip << ":" << ntohs(client_addr.sin_port) << std::endl;
    }
}

Gestion des clients existants:

cppCopiervoid checkExistingClients(struct pollfd fds[], int nfds, std::map<int, ClientActivity>& client_activity) {
    for (int i = 1; i < nfds; i++) {
        if (fds[i].revents == 0) {
            continue; // Rien à traiter pour ce client
        }
        
        int client_fd = fds[i].fd;
        
        // Mettre à jour le timestamp d'activité
        client_activity[client_fd].last_activity = time(NULL);
        
        if (fds[i].revents & POLLIN) {
            // Prêt à lire
            char buffer[BUFFER_SIZE + 1];
            int bytes = recv(client_fd, buffer, BUFFER_SIZE, 0);
            
            if (bytes <= 0) {
                if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    // Pas de données disponibles, réessayer plus tard
                    continue;
                }
                
                // Connexion fermée ou erreur
                close(client_fd);
                client_activity.erase(client_fd);
                removeClient(fds, &nfds, i);
                i--; // Ajuster l'index après suppression
            } else {
                // Données reçues
                buffer[bytes] = '\0';
                
                // Ajouter au buffer de requête du client
                if (client_buffers.find(client_fd) == client_buffers.end()) {
                    client_buffers[client_fd] = "";
                }
                client_buffers[client_fd] += buffer;
                
                // Vérifier si la requête est complète
                if (isRequestComplete(client_buffers[client_fd])) {
                    // Traiter la requête
                    HttpRequest request;
                    if (parseRequest(client_buffers[client_fd], request)) {
                        // Préparer la réponse
                        HttpResponse response = processRequest(request);
                        
                        // Stocker la réponse pour envoi
                        client_responses[client_fd] = response.build();
                        
                        // Configurer pour écriture
                        fds[i].events = POLLOUT;
                        client_activity[client_fd].has_pending_data = true;
                    } else {
                        // Requête malformée
                        HttpResponse error_response;
                        error_response.setStatus(400);
                        error_response.setBody("Bad Request", "text/plain");
                        
                        client_responses[client_fd] = error_response.build();
                        fds[i].events = POLLOUT;
                        client_activity[client_fd].has_pending_data = true;
                    }
                    
                    // Réinitialiser le buffer après traitement
                    client_buffers[client_fd].clear();
                }
            }
        }
        
        if (fds[i].revents & POLLOUT) {
            // Prêt à écrire
            if (client_responses.find(client_fd) != client_responses.end() && 
                !client_responses[client_fd].empty()) {
                
                int bytes = send(client_fd, client_responses[client_fd].c_str(), 
                               client_responses[client_fd].size(), 0);
                
                if (bytes <= 0) {
                    if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                        // Impossible d'envoyer maintenant, réessayer plus tard
                        continue;
                    }
                    
                    // Connexion fermée ou erreur
                    close(client_fd);
                    client_activity.erase(client_fd);
                    client_buffers.erase(client_fd);
                    client_responses.erase(client_fd);
                    removeClient(fds, &nfds, i);
                    i--; // Ajuster l'index après suppression
                } else if (bytes < client_responses[client_fd].size()) {
                    // Envoi partiel
                    client_responses[client_fd] = client_responses[client_fd].substr(bytes);
                } else {
                    // Envoi complet
                    client_responses[client_fd].clear();
                    
                    // Revenir à la lecture pour la prochaine requête
                    fds[i].events = POLLIN;
                    client_activity[client_fd].has_pending_data = false;
                    
                    // Si Connection: close était spécifié, fermer la connexion
                    if (shouldCloseConnection(client_fd)) {
                        close(client_fd);
                        client_activity.erase(client_fd);
                        client_buffers.erase(client_fd);
                        client_responses.erase(client_fd);
                        removeClient(fds, &nfds, i);
                        i--; // Ajuster l'index après suppression
                    }
                }
            }
        }
        
        if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            // Erreur sur le socket
            close(client_fd);
            client_activity.erase(client_fd);
            client_buffers.erase(client_fd);
            client_responses.erase(client_fd);
            removeClient(fds, &nfds, i);
            i--; // Ajuster l'index après suppression
        }
    }
}

Suppression d'un client:

cppCopiervoid removeClient(struct pollfd fds[], int *nfds, int index) {
    // Déplacer le dernier client à la position libérée
    if (index < (*nfds - 1)) {
        fds[index] = fds[*nfds - 1];
    }
    
    // Réduire le compteur
    (*nfds)--;
    
    // Effacer le dernier slot
    memset(&fds[*nfds], 0, sizeof(struct pollfd));
}

Vérification de complétude de requête:

cppCopierbool isRequestComplete(const std::string& request_buffer) {
    // Vérifier si nous avons reçu la fin des headers
    size_t header_end = request_buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return false; // Pas encore reçu la fin des headers
    }
    
    // Vérifier s'il y a un Content-Length
    size_t content_length_pos = request_buffer.find("Content-Length: ");
    if (content_length_pos == std::string::npos) {
        return true; // Pas de body attendu
    }
    
    // Extraire la valeur de Content-Length
    size_t value_start = content_length_pos + 16; // Longueur de "Content-Length: "
    size_t value_end = request_buffer.find("\r\n", value_start);
    size_t content_length = std::stoul(request_buffer.substr(value_start, value_end - value_start));
    
    // Vérifier si nous avons reçu tout le body
    return request_buffer.size() >= (header_end + 4 + content_length);
}
Points de validation

 Le serveur accepte plusieurs connexions simultanées
 Les données sont bien reçues des clients
 Les connexions fermées sont bien nettoyées
 Pas de fuite de file descriptors
 Les timeouts sont correctement gérés
 Les requêtes partielles sont traitées correctement
 Les connexions persistantes fonctionnent (keep-alive)
 Les erreurs POLLERR, POLLHUP et POLLNVAL sont gérées

Tests recommandés

Test de connexions multiples:

bashCopier# Script pour ouvrir plusieurs connexions simultanées
for i in {1..100}; do
  (echo -e "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080 &)
done

Test de timeout:

bashCopier# Ouvre une connexion mais n'envoie rien
nc localhost 8080
# Attendre plus que le timeout configuré (60s)
# Vérifier que le serveur ferme la connexion

Test de requête partielle:

bashCopier# Envoyer une partie de requête
echo -n "GET / HTTP/1.1\r\nHost: localhost" | nc localhost 8080
# Vérifier que la connexion reste ouverte

Test de connexion persistante:

bashCopierecho -e "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\nGET /about HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n" | nc localhost 8080
# Vérifier que deux réponses sont reçues
Bonnes pratiques

Limiter le nombre maximal de clients (MAX_CLIENTS)
Implémenter un timeout d'inactivité configurable
Journaliser les connexions/déconnexions et les erreurs
Utiliser des buffers séparés pour chaque client
Éviter les blocages pendant le traitement des requêtes
Gérer correctement les envois partiels