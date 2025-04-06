#ifndef SERVER_HPP
# define SERVER_HPP

# include "utils/Common.hpp"
# include "socket/Socket.hpp"
# include "http/HttpRequest.hpp"
# include "http/HttpResponse.hpp"
# include "http/ResponseHandler.hpp"
# include "http/RouteHandler.hpp"
# include "config/ConfigTypes.hpp"

# define MAX_CLIENTS 1024

class Server
{
  private:
	Socket server_socket;      // Socket principal du serveur
	int port;                  // Port d'écoute
	struct pollfd fds[MAX_CLIENTS]; // Tableau des descripteurs pour poll()
	int nfds;                  // Nombre de descripteurs actifs
	bool running;              // État d'exécution du serveur
	ServerConfig server_config; // Configuration du serveur
	RouteHandler route_handler; // Gestionnaire de routes

	// Méthodes privées
	void setupSignalHandlers();  // Configuration des gestionnaires de signal
	void cleanupResources();     // Libération des ressources
	bool handleEvent(int index); // Gestion des événements poll
	void sendHttpResponse(int client_fd, const HttpRequest& request); // Envoi d'une réponse HTTP

  public:
	Server(int port, const ServerConfig& config);
	~Server();

	void start();  // Démarrage du serveur
	void stop();   // Arrêt du serveur

	// Méthodes pour gérer les connections
	void acceptNewConnection();        // Accepter une nouvelle connexion client
	void handleClientData(int client_index); // Traiter les données reçues d'un client

	// Gestionnaire de signal statique
	static void signalHandler(int signal);
	static Server *instance;  // Instance unique pour le gestionnaire de signal
};

#endif