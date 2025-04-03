#include "Server.hpp"
#include "utils/Common.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
	int port = DEFAULT_PORT;
	
	// Vérifier si un port a été fourni en argument
	if (argc > 1)
	{
		int tmp_port = std::atoi(argv[1]);
		if (tmp_port > 0 && tmp_port < 65536) {
			port = tmp_port;
		} else {
			LOG_WARNING("Invalid port number. Using default port " << DEFAULT_PORT);
		}
	}

	// Créer et démarrer le serveur
	try {
		Server server(port);
		server.start();
	} catch (const std::exception& e) {
		LOG_ERROR("Error: " << e.what());
		return 1;
	}
	
	return 0;
}