#include "../include/Server.hpp"

// Initialisation de la variable statique
Server* Server::instance = NULL;

Server::Server(int port) : port(port), nfds(1), running(false) {
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
	// Les sockets sont fermés automatiquement par le destructeur de Socket
	for (int i = 1; i < nfds; i++) {
		if (fds[i].fd >= 0) {
			::close(fds[i].fd);
			fds[i].fd = -1;
		}
	}
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
		
		std::cout << "Server started on port " << port << std::endl;
		
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
		std::cerr << "Server error: " << e.what() << std::endl;
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
			::close(fds[index].fd);
			fds[index].fd = -1;
			
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
		Socket client_socket = server_socket.accept();
		
		// Ignorer les accept qui ont échoué en mode non-bloquant
		if (client_socket.getFd() < 0) {
			return;
		}
		
		// Configuration non-bloquante pour le client
		client_socket.setNonBlocking(true);
		
		// Vérifier si on a atteint le nombre maximum de clients
		if (nfds >= MAX_CLIENTS) {
			std::cerr << "Too many clients!" << std::endl;
			return;
		}
		
		// Ajouter le client au tableau poll
		fds[nfds].fd = client_socket.getFd();
		fds[nfds].events = POLLIN;
		fds[nfds].revents = 0;
		nfds++;
		
		std::cout << "New client connected, fd: " << client_socket.getFd() << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "Error accepting connection: " << e.what() << std::endl;
	}
}

void Server::handleClientData(int client_index) {
	char buffer[BUFFER_SIZE];
	
	int nbytes = recv(fds[client_index].fd, buffer, sizeof(buffer) - 1, 0);
	
	if (nbytes <= 0) {
		if (nbytes < 0) {
			std::cerr << "Error reading from client: " << strerror(errno) << std::endl;
		}
		
		// Fermer la connexion
		::close(fds[client_index].fd);
		fds[client_index].fd = -1;
		
		// Compacter le tableau
		for (int j = client_index; j < nfds - 1; j++) {
			fds[j] = fds[j + 1];
		}
		nfds--;
		
		std::cout << "Client disconnected" << std::endl;
		return;
	}
	
	// Null-terminer le buffer pour le traiter comme une chaîne
	buffer[nbytes] = '\0';
	
	// Pour l'instant, juste écho des données reçues (sera remplacé par le parsing HTTP)
	std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!";
	send(fds[client_index].fd, response.c_str(), response.length(), 0);
	
	std::cout << "Received data from client: " << buffer << std::endl;
}