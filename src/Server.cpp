#include "../include/Server.hpp"

// Initialisation de la variable statique
Server* Server::instance = NULL;

Server::Server(int port) : port(port), nfds(1), running(false), route_handler("www") {
	// Initialiser le tableau fds
	memset(fds, 0, sizeof(fds));
	
	// Enregistrer l'instance pour le gestionnaire de signal
	instance = this;
}

Server::~Server() {
	cleanupResources();
}

void Server::setupSignalHandlers() {
	struct sigaction sa;
	sa.sa_handler = signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	
	sigaction(SIGINT, &sa, NULL);  // Ctrl+C
	sigaction(SIGTERM, &sa, NULL); // Signal de terminaison
}

void Server::signalHandler(int signal) {
	std::cout << "Received signal " << signal << ", shutting down gracefully..." << std::endl;
	if (instance) {
		instance->stop();
	}
}

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
	
	std::cout << "✓ All client connections closed cleanly" << std::endl;
}

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
		
		std::cout << "✅ Server started successfully on port " << port << std::endl;
		
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
		std::cerr << "❌ Server error: " << e.what() << std::endl;
		cleanupResources();
		throw;
	}
}

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
			std::cout << "➖ Client socket closed due to error/hangup, fd: " << fds[index].fd << std::endl;
			
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

void Server::stop() {
	running = false;
}

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
		int flags = fcntl(client_fd, F_GETFL, 0);
		if (flags < 0) {
			::close(client_fd);
			throw std::runtime_error("Failed to get socket flags: " + std::string(strerror(errno)));
		}
		
		flags |= O_NONBLOCK;
		if (fcntl(client_fd, F_SETFL, flags) < 0) {
			::close(client_fd);
			throw std::runtime_error("Failed to set socket flags: " + std::string(strerror(errno)));
		}
		
		// Vérifier si on a atteint le nombre maximum de clients
		if (nfds >= MAX_CLIENTS) {
			std::cerr << "⚠️ Maximum client limit reached, rejecting connection" << std::endl;
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
		
		std::cout << "➕ New client connected from " << client_ip << ", fd: " << client_fd 
				  << ", total clients: " << (nfds-1) << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "❌ Error accepting connection: " << e.what() << std::endl;
	}
}

void Server::handleClientData(int client_index) {
	char buffer[BUFFER_SIZE];
	int client_fd = fds[client_index].fd;
	
	// Recevoir les données
	int nbytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	
	// Traiter le cas d'erreur ou de déconnexion
	if (nbytes <= 0) {
		if (nbytes < 0) {
			std::cerr << "❌ Error reading from client: " << strerror(errno) << std::endl;
		}
		
		// Fermer le socket client
		::close(client_fd);
		
		// Compacter le tableau fds
		for (int j = client_index; j < nfds - 1; j++) {
			fds[j] = fds[j + 1];
		}
		nfds--;
		
		std::cout << "➖ Client disconnected, fd: " << client_fd 
				 << ", remaining clients: " << (nfds-1) << std::endl;
		return;
	}
	
	// Null-terminer le buffer pour le traiter comme une chaîne
	buffer[nbytes] = '\0';
	
	// Créer et parser la requête HTTP
	HttpRequest request;
	std::string raw_request(buffer, nbytes);
	
	if (!request.parse(raw_request)) {
		// Requête malformée
		std::cerr << "❌ Malformed HTTP request" << std::endl;
		HttpResponse error_response = createErrorResponse(400, "Bad Request");
		ResponseHandler::sendResponse(client_fd, error_response, request);
		return;
	}
	
	// Envoyer une réponse HTTP
	sendHttpResponse(client_fd, request);
	
	// Log des données reçues (la première ligne de la requête)
	std::cout << "📩 Received " << request.getMethod() << " request for: " << request.getUri() << std::endl;
}

// Mise à jour de la méthode d'envoi de réponse HTTP
void Server::sendHttpResponse(int client_fd, const HttpRequest& request) {
	// Utiliser le gestionnaire de routes pour traiter la requête
	HttpResponse response = route_handler.processRequest(request);
	
	// Envoyer la réponse au client
	ssize_t bytes_sent = ResponseHandler::sendResponse(client_fd, response, request);
	
	// Log de la réponse envoyée
	std::cout << "📤 Sent HTTP " << response.getStatusCode() << " response: " 
			  << bytes_sent << " bytes" << std::endl;
}