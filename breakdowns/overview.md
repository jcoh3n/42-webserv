# 🚀 Webserv - Guide d'implémentation étape par étape (Mandatory)

## 📋 Table des matières
1. [Prérequis](#-prérequis)
2. [Phase 1 - Core du serveur](#-phase-1---core-du-serveur)
3. [Phase 2 - HTTP Basics](#-phase-2---http-basics)
4. [Phase 3 - Gestion des fichiers](#-phase-3---gestion-des-fichiers)
5. [Phase 4 - Configuration](#-phase-4---configuration)
6. [Phase 5 - Méthodes avancées](#-phase-5---méthodes-avancées)
7. [Phase 6 - CGI](#-phase-6---cgi)
8. [Phase 7 - Tests et validation](#-phase-7---tests-et-validation)
9. [Phase 8 - Optimisation et gestion des erreurs](#-phase-8---optimisation-et-gestion-des-erreurs)
10. [Checklist finale](#-checklist-finale)

---

## 🛠️ Prérequis
- **Compilation**: C++98 avec flags `-Wall -Wextra -Werror`
- **Connaissances**: 
  - Bases de C++ (sans C++11+)
  - Protocole HTTP/1.1
  - Programmation système (sockets, file descriptors)

---

## 🔌 Phase 1 - Core du serveur

### Étape 1.1 - Initialisation des sockets
```cpp
// Création du socket principal
int server_fd = socket(AF_INET, SOCK_STREAM, 0);

// Configuration
struct sockaddr_in address;
address.sin_family = AF_INET;
address.sin_addr.s_addr = INADDR_ANY;
address.sin_port = htons(8080); // Port par défaut

// Binding
bind(server_fd, (struct sockaddr*)&address, sizeof(address));
listen(server_fd, SOMAXCONN); // File d'attente
Étape 1.2 - Boucle événementielle
cppCopierstruct pollfd fds[MAX_CLIENTS];
fds[0] = {server_fd, POLLIN, 0}; // Surveiller le socket principal

while (true) {
    int ready = poll(fds, nfds, -1); // Attente infinie
    if (ready == -1) {
        // Gestion d'erreur
        continue;
    }
    
    // Gestion des nouveaux clients
    if (fds[0].revents & POLLIN) {
        int client_fd = accept(server_fd, NULL, NULL);
        fcntl(client_fd, F_SETFL, O_NONBLOCK); // Mode non-bloquant
        // Ajouter à pollfd
    }
}

🌐 Phase 2 - HTTP Basics
Étape 2.1 - Parsing des requêtes
Structure à analyser:
CopierGET /index.html HTTP/1.1\r\n
Host: localhost\r\n
Connection: keep-alive\r\n
\r\n
[body si POST]
Points clés:

Détecter \r\n\r\n pour séparer headers/body
Extraire:

Méthode (GET/POST/DELETE)
URI demandée
Headers importants (Content-Length, Host)



Étape 2.2 - Réponses de base
cppCopierstd::string build_response(int code, const std::string& body) {
    std::stringstream response;
    response << "HTTP/1.1 " << code << " OK\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "\r\n"
             << body;
    return response.str();
}

📂 Phase 3 - Gestion des fichiers
Étape 3.1 - Servir des fichiers
cppCopierstd::ifstream file("www" + path, std::ios::binary);
if (!file) return 404; // Not Found

std::string content((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
return build_response(200, content);
Étape 3.2 - Gestion des répertoires
cppCopierif (is_directory(path)) {
    if (path.back() != '/') return 301; // Redirection
    path += "index.html"; // Fichier par défaut
    // Essayer de servir le fichier
}

⚙️ Phase 4 - Configuration
Format de configuration minimal
nginxCopierserver {
    listen 8080;
    server_name localhost;
    root /var/www;
    client_max_body_size 1M;
    
    location / {
        methods GET POST;
        index index.html;
    }
}
Points clés à parser:

Blocs server imbriqués
Directives listen, root, client_max_body_size
Routes avec méthodes autorisées


🔄 Phase 5 - Méthodes avancées
POST (Upload)
cppCopierstd::ofstream outfile("uploads/" + filename, std::ios::binary);
outfile.write(body.data(), body.size());
return 201; // Created
DELETE
cppCopierif (unlink(path.c_str()) == 0) {
    return 204; // No Content
} else {
    return 404; // Not Found
}

🖥️ Phase 6 - CGI
Workflow de base:

Détecter l'extension (.php)
Créer des pipes pour communication
Fork + execve
Écrire le body dans stdin du CGI
Lire stdout et renvoyer au client

cppCopier// Variables d'environnement
env["PATH_INFO"] = path;
env["QUERY_STRING"] = query;

// Exécution
execve("/usr/bin/php-cgi", args, env_vars);

🧪 Phase 7 - Tests et validation
Types de tests:

Tests unitaires (composants individuels)
Tests d'intégration (interactions entre composants)
Tests de conformité HTTP (comparaison avec NGINX)
Tests de charge (ab, wrk)
Tests de résilience (requêtes malformées)


⚡ Phase 8 - Optimisation et gestion des erreurs
Points d'optimisation:

Utilisation de sendfile() pour les gros fichiers
Cache des fichiers fréquemment accédés
Gestion optimisée des connexions persistantes

Gestion des erreurs:

Validation rigoureuse des entrées
Récupération gracieuse après erreurs
Journalisation détaillée mais non bloquante


✅ Checklist finale

 Serveur multi-port fonctionnel
 Gestion non-bloquante avec poll()
 Parsing HTTP complet
 GET/POST/DELETE implémentées
 Fichiers statiques + gestion erreurs
 Système de configuration
 CGI basique fonctionnel
 Tests passant avec différents clients
 Résilience face aux requêtes malformées
 Pas de fuites mémoire (validé avec valgrind)
 Comportement conforme à HTTP/1.1



Note importante: Toujours vérifier que le serveur ne crash jamais, même avec des requêtes malformées ou sous forte charge. Utiliser valgrind pour détecter les fuites mémoire.