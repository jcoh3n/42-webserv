#include "../include/Server.hpp"
#include "http/ResponseHandler.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "utils/Common.hpp"
#include <cstring>

/**
 * @brief Constructeur de la classe Server
 * @param port Le port sur lequel le serveur va écouter
 * @param config La configuration du serveur
 * 
 * Initialise le serveur avec un port spécifique, prépare les descripteurs
 * de fichier pour poll().
 */
Server::Server(int port, const ServerConfig& config) 
    : server_socket()
    , port(port)
    , nfds(1)
    , running(false)
    , server_config(config)
    , route_handler(config.root_directory, config) {
	// Initialiser le tableau fds
	memset(fds, 0, sizeof(fds));
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
	std::string raw_request;
	size_t content_length = 0;
	bool headers_complete = false;
	int retry_count = 0;
	const int MAX_RETRIES = 1000; // Protection contre les boucles infinies
	
	while (true) {
		// Recevoir les données
		int nbytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
		
		if (nbytes < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// Données non disponibles, attendre un peu et réessayer
				retry_count++;
				if (retry_count > MAX_RETRIES) {
					LOG_ERROR("Max retries reached while reading from client");
					break;
				}
				usleep(1000); // Attendre 1ms avant de réessayer
				continue;
			}
			LOG_ERROR("Error reading from client: " << strerror(errno));
			break;
		} else if (nbytes == 0) {
			// Client a fermé la connexion
			LOG_NETWORK("Client closed connection, fd: " << client_fd);
			break;
		}
		
		retry_count = 0; // Réinitialiser le compteur après une lecture réussie
		raw_request.append(buffer, nbytes);
		
		// Si nous n'avons pas encore trouvé la fin des headers
		if (!headers_complete) {
			size_t headers_end = raw_request.find("\r\n\r\n");
			if (headers_end != std::string::npos) {
				headers_complete = true;
				
				// Chercher Content-Length dans les headers
				size_t cl_pos = raw_request.find("Content-Length: ");
				if (cl_pos != std::string::npos) {
					size_t cl_end = raw_request.find("\r\n", cl_pos);
					if (cl_end != std::string::npos) {
						std::string cl_str = raw_request.substr(cl_pos + 16, cl_end - (cl_pos + 16));
						std::stringstream ss(cl_str);
						ss >> content_length;
						
						LOG_INFO("Found Content-Length: " << content_length);
						
						// Extraire l'URI pour vérifier la limite de taille
						size_t uri_start = raw_request.find(" ") + 1;
						size_t uri_end = raw_request.find(" ", uri_start);
						if (uri_start != std::string::npos && uri_end != std::string::npos) {
							std::string uri = raw_request.substr(uri_start, uri_end - uri_start);
							const LocationConfig* location = route_handler.findMatchingLocation(uri);
							size_t max_body_size = location ? location->client_max_body_size : 1024 * 1024; // 1MB par défaut
							
							if (content_length > max_body_size) {
								LOG_ERROR("Content length exceeds maximum allowed size");
								HttpResponse error_response = HttpResponse::createError(413, "Request Entity Too Large");
								ResponseHandler::sendResponse(client_fd, error_response, HttpRequest());
								break;
							}
						}
					}
				}
			}
		}
		
		// Si nous avons les headers complets et toutes les données du body
		if (headers_complete && content_length > 0) {
			size_t headers_end = raw_request.find("\r\n\r\n");
			size_t total_expected = headers_end + 4 + content_length;
			
			if (raw_request.length() >= total_expected) {
				// Nous avons toute la requête
				break;
			}
		} else if (headers_complete && content_length == 0) {
			// Requête sans body
			break;
		}
	}
	
	// Fermer la connexion si une erreur s'est produite
	if (raw_request.empty()) {
		::close(client_fd);
		for (int j = client_index; j < nfds - 1; j++) {
			fds[j] = fds[j + 1];
		}
		nfds--;
		LOG_NETWORK("Client disconnected, fd: " << client_fd 
				  << ", remaining clients: " << (nfds-1));
		return;
	}
	
	// Traiter la requête
	try {
		HttpRequest request;
		if (request.parse(raw_request)) {
			LOG_INFO("➜ " << request.getMethod() << " " << request.getUri() << " (Processing)");
			sendHttpResponse(client_fd, request);
		} else {
			LOG_ERROR("Malformed HTTP request");
			HttpResponse error_response = HttpResponse::createError(400, "Bad Request");
			ResponseHandler::sendResponse(client_fd, error_response, request);
		}
	} catch (const std::exception& e) {
		LOG_ERROR("Error processing request: " << e.what());
		HttpRequest dummy_request;
		HttpResponse error_response = HttpResponse::createError(500, "Internal Server Error");
		ResponseHandler::sendResponse(client_fd, error_response, dummy_request);
	}
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