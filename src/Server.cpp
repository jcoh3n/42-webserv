#include "../include/Server.hpp"
#include "http/ResponseHandler.hpp"
#include "http/HttpRequest.hpp"
#include "utils/Common.hpp"
#include <csignal>
#include <cstring>

// Initialisation de la variable statique
Server* Server::instance = NULL;

/**
 * @brief Constructeur de la classe Server
 * @param port Le port sur lequel le serveur va écouter
 * @param config La configuration du serveur
 * 
 * Initialise le serveur avec un port spécifique, prépare les descripteurs
 * de fichier pour poll() et initialise l'instance statique pour le gestionnaire de signal.
 */
Server::Server(int port, const ServerConfig& config) 
    : port(port)
    , nfds(1)
    , running(false)
    , server_config(config)
    , route_handler(config.root_directory, config) {
	// Initialiser le tableau fds
	memset(fds, 0, sizeof(fds));
	
	// Enregistrer l'instance pour le gestionnaire de signal
	instance = this;
}

/**
 * @brief Destructeur de la classe Server
 * 
 * Nettoie les ressources avant de détruire l'objet.
 */
Server::~Server() {
	cleanupResources();
}

/**
 * @brief Configure les gestionnaires de signaux
 * 
 * Met en place les handlers pour SIGINT et SIGTERM pour permettre
 * un arrêt propre du serveur lorsqu'il reçoit un de ces signaux.
 */
void Server::setupSignalHandlers() {
	struct sigaction sa;
	sa.sa_handler = signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	
	sigaction(SIGINT, &sa, NULL);  // Ctrl+C
	sigaction(SIGTERM, &sa, NULL); // Signal de terminaison
}

/**
 * @brief Gestionnaire statique des signaux
 * @param signal Le numéro du signal reçu
 * 
 * Appelé lorsqu'un signal est reçu par le processus,
 * cette fonction arrête proprement le serveur.
 */
void Server::signalHandler(int signal) {
	LOG_INFO("Received signal " << signal << ", shutting down gracefully...");
	if (instance) {
		instance->stop();
	}
}

/**
 * @brief Nettoie les ressources utilisées par le serveur
 * 
 * Ferme tous les descripteurs de fichiers des clients
 * et réinitialise le compteur de descripteurs.
 */
void Server::cleanupResources() {
	// Fermer tous les sockets clients
	for (int i = 1; i < nfds; i++) {
		if (fds[i].fd >= 0) {
			::close(fds[i].fd);
			fds[i].fd = -1;
		}
	}
	// Réinitialiser le nombre de descripteurs
	nfds = 1;
	
	LOG_SUCCESS("All client connections closed cleanly");
}

/**
 * @brief Démarre le serveur
 * 
 * Initialise le socket serveur, configure poll(), et entre dans
 * la boucle principale d'événements. Cette méthode est bloquante
 * jusqu'à ce que le serveur soit arrêté.
 */
void Server::start() {
	try {
		// Création et configuration du socket serveur
		server_socket.create();
		server_socket.setReuseAddr(true);
		server_socket.bind(port);
		server_socket.listen();
		server_socket.setNonBlocking(true);
		
		// Initialisation du poll pour le socket principal
		fds[0].fd = server_socket.getFd();
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		nfds = 1;
		
		// Configuration des gestionnaires de signaux
		setupSignalHandlers();
		
		LOG_SUCCESS("Server started successfully on port " << port);
		LOG_INFO("Server running at " << BLUE << BOLD << "http://localhost:" << port << RESET);
		
		running = true;
		
		// Boucle principale (poll)
		while (running) {
			int ret = poll(fds, nfds, -1); // Attente infinie
			if (ret < 0) {
				if (errno == EINTR) {
					// Interruption par un signal, vérifier si on doit s'arrêter
					continue;
				}
				throw std::runtime_error("Poll failed: " + std::string(strerror(errno)));
			}
			
			// Traitement des événements
			for (int i = 0; i < nfds; i++) {
				if (fds[i].revents == 0) {
					continue; // Aucun événement pour ce fd
				}
				
				if (handleEvent(i) == false) {
					i--; // Si le fd a été supprimé, on décrémente i pour ne pas sauter le prochain
				}
			}
		}
	} catch (const std::exception& e) {
		LOG_ERROR("Server error: " << e.what());
		cleanupResources();
		throw;
	}
}

/**
 * @brief Gère un événement détecté par poll()
 * @param index L'index du descripteur de fichier dans le tableau fds
 * @return true si le descripteur est toujours valide, false s'il a été supprimé
 * 
 * Traite les événements de lecture (POLLIN) et d'erreur sur les sockets.
 * Pour le socket serveur (index 0), accepte de nouvelles connexions.
 * Pour les sockets clients, lit et traite les données reçues.
 */
bool Server::handleEvent(int index) {
	// Traiter les différents événements possibles
	if (fds[index].revents & POLLIN) {
		if (index == 0) {
			// Nouvelle connexion sur le socket serveur
			acceptNewConnection();
		} else {
			// Données à lire sur un socket client
			handleClientData(index);
		}
	}
	
	if (fds[index].revents & (POLLERR | POLLHUP | POLLNVAL)) {
		// Erreur ou connexion fermée
		if (index > 0) { // Ne pas fermer le socket serveur
			// Fermer le socket client
			::close(fds[index].fd);
			LOG_NETWORK("Client socket closed due to error/hangup, fd: " << fds[index].fd);
			
			// Compacter le tableau
			for (int j = index; j < nfds - 1; j++) {
				fds[j] = fds[j + 1];
			}
			nfds--;
			return false; // Indiquer que le fd a été supprimé
		}
	}
	
	return true;
}

/**
 * @brief Arrête le serveur
 * 
 * Marque le serveur comme non-exécutable, ce qui entraînera
 * la sortie de la boucle principale dans start().
 */
void Server::stop() {
	running = false;
}

/**
 * @brief Accepte une nouvelle connexion client
 * 
 * Utilise l'appel système accept() pour établir une nouvelle connexion.
 * Configure le socket client en mode non-bloquant et l'ajoute au tableau poll.
 */
void Server::acceptNewConnection() {
	try {
		// Accepter directement la connexion avec l'appel système accept()
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		
		int client_fd = ::accept(server_socket.getFd(), (struct sockaddr*)&client_addr, &client_len);
		
		if (client_fd < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				throw std::runtime_error("Accept failed: " + std::string(strerror(errno)));
			}
			return;
		}
		
		// Configuration non-bloquante pour le client
		int flags = fcntl(client_fd, F_GETFL, 0); // Récupérer les options existantes F_GETFL
		if (flags < 0) {
			::close(client_fd);
			throw std::runtime_error("Failed to get socket flags: " + std::string(strerror(errno)));
		}
		
		// Ajouter l'option non-bloquante
		flags |= O_NONBLOCK;
		if (fcntl(client_fd, F_SETFL, flags) < 0) { // Appliquer les nouvelles options F_SETFL
			::close(client_fd);
			throw std::runtime_error("Failed to set socket flags: " + std::string(strerror(errno)));
		}
		
		// Vérifier si on a atteint le nombre maximum de clients
		if (nfds >= MAX_CLIENTS) {
			LOG_WARNING("Maximum client limit reached, rejecting connection");
			::close(client_fd);
			return;
		}
		
		// Ajouter le client au tableau poll
		fds[nfds].fd = client_fd;
		fds[nfds].events = POLLIN;
		fds[nfds].revents = 0;
		nfds++;
		
		// Obtenir l'adresse IP du client pour les logs
		char client_ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
		
		LOG_NETWORK("New client connected from " << client_ip << ", fd: " << client_fd 
				  << ", total clients: " << (nfds-1));
	} catch (const std::exception& e) {
		LOG_ERROR("Error accepting connection: " << e.what());
	}
}

/**
 * @brief Traite les données reçues d'un client
 * @param client_index L'index du descripteur de fichier client dans le tableau fds
 * 
 * Lit les données du socket client, parse la requête HTTP,
 * et envoie une réponse appropriée.
 */
void Server::handleClientData(int client_index) {
	char buffer[BUFFER_SIZE];
	int client_fd = fds[client_index].fd;
	
	// Recevoir les données
	int nbytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	
	// Traiter le cas d'erreur ou de déconnexion
	if (nbytes <= 0) {
		// Cas: nbytes < 0: une erreur est survenue
		if (nbytes < 0) {
			LOG_ERROR("Error reading from client: " << strerror(errno));
		}
		// Cas: nbytes = 0: le client a fermé la connexion
		// Fermer le socket client
		::close(client_fd);
		
		// Compacter le tableau fds
		for (int j = client_index; j < nfds - 1; j++) {
			fds[j] = fds[j + 1];
		}
		nfds--;
		
		LOG_NETWORK("Client disconnected, fd: " << client_fd 
				 << ", remaining clients: " << (nfds-1));
		return;
	}
	
	// '\0' -> le buffer pour le traiter comme une chaîne
	buffer[nbytes] = '\0';
	
	// Créer et parser la requête HTTP
	HttpRequest request;
	std::string raw_request(buffer, nbytes); // Convertir le buffer en chaîne de caractères
	
	if (!request.parse(raw_request)) {
		// Requête malformée
		LOG_ERROR("Malformed HTTP request");
		HttpResponse error_response = createErrorResponse(400, "Bad Request");
		ResponseHandler::sendResponse(client_fd, error_response, request);
		return;
	}
	
	// Envoyer une réponse HTTP
	sendHttpResponse(client_fd, request);
}

/**
 * @brief Envoie une réponse HTTP au client
 * @param client_fd Le descripteur de fichier du client
 * @param request La requête HTTP reçue
 * 
 * Utilise le gestionnaire de routes pour traiter la requête,
 * puis envoie la réponse générée au client.
 */
void Server::sendHttpResponse(int client_fd, const HttpRequest& request) {
	// Log de la requête reçue
	LOG_REQUEST(request.getMethod(), request.getUri(), "Processing");
	
	// Utiliser le gestionnaire de routes pour traiter la requête
	HttpResponse response = route_handler.processRequest(request);
	
	// Envoyer la réponse au client
	ssize_t bytes_sent = ResponseHandler::sendResponse(client_fd, response, request);
	
	// Log de la réponse envoyée
	LOG_RESPONSE(response.getStatusCode(), bytes_sent);
}