#include "../include/Server.hpp"
#include <cstdlib>

int	main(int argc, char **argv)
{
	try
	{
		int port = DEFAULT_PORT; // Port par défaut

		// Vérifier si un port est spécifié en argument
		if (argc > 1)
		{
			port = std::atoi(argv[1]);
			if (port <= 0 || port > 65535)
			{
				std::cerr << "Invalid port number. Using default port " << DEFAULT_PORT << "." << std::endl;
				port = DEFAULT_PORT;
			}
		}

		// Créer et démarrer le serveur
		Server server(port);
		server.start();

		return (0);
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}
}