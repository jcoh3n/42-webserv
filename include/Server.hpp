#ifndef SERVER_HPP
# define SERVER_HPP

# include "utils/Common.hpp"
# include "socket/Socket.hpp"

# define MAX_CLIENTS 1024

class Server
{
  private:
	Socket server_socket;
	int port;
	struct pollfd fds[MAX_CLIENTS];
	int nfds;
	bool running;

	// Méthodes privées
	void setupSignalHandlers();
	void cleanupResources();
	bool handleEvent(int index);

  public:
	Server(int port);
	~Server();

	void start();
	void stop();

	// Méthodes pour gérer les connections
	void acceptNewConnection();
	void handleClientData(int client_index);

	// Gestionnaire de signal statique
	static void signalHandler(int signal);
	static Server *instance;
	// Pour accéder à l'instance depuis le gestionnaire de signal
};

#endif