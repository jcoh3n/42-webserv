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

/**
 * @brief Serveur HTTP gérant les connexions clients et le traitement des requêtes
 * 
 * Cette classe implémente un serveur HTTP capable de gérer les connexions clients
 * et le traitement des requêtes HTTP, optimisé pour une utilisation avec un
 * mécanisme de polling centralisé.
 */
class Server
{
  private:
	Socket server_socket;      // Socket principal du serveur
	int port;                  // Port d'écoute
	bool running;              // État d'exécution du serveur
	ServerConfig server_config; // Configuration du serveur
	RouteHandler route_handler; // Gestionnaire de routes
    std::map<int, std::string> client_requests; // Stockage des requêtes en cours par fd

	// Méthodes privées
	void sendHttpResponse(int client_fd, const HttpRequest& request); // Envoi d'une réponse HTTP
    void processCompleteRequest(int client_fd, const std::string& raw_request); // Traitement d'une requête complète

  public:
	Server(int port, const ServerConfig& config);
	~Server();

    // Méthodes de contrôle
    void initialize();         // Initialiser le socket du serveur
    void stop();               // Arrêter le serveur

	// Méthodes pour gérer les connections
	int acceptNewConnection();  // Accepter une nouvelle connexion client, retourne le nouveau fd
	bool handleClientData(int client_fd); // Traiter les données reçues d'un client, retourne false si la connexion doit être fermée
    void closeClientConnection(int client_fd); // Ferme une connexion client
    void handleClientTimeout(int client_fd); // Gère un timeout de client
    
    // Accesseurs
    int getPort() const { return port; }
    int getSocketFd() const; // Récupérer le descripteur de fichier du socket serveur
    bool isRunning() const { return running; }
    bool matchesSocketFd(int fd) const; // Vérifier si le fd correspond au socket du serveur
    const ServerConfig& getConfig() const { return server_config; }
};

#endif