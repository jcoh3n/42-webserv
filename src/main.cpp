#include "Server.hpp"
#include "utils/Common.hpp"
#include "config/ConfigParser.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
	// Vérifier qu'un fichier de configuration a été fourni
	if (argc < 2) {
		LOG_ERROR("Usage: " << argv[0] << " <config_file>");
		LOG_ERROR("Configuration file required");
		return 1;
	}
	
	std::string config_file = argv[1];
	LOG_INFO("Using config file: " << config_file);
	
	// Charger la configuration
	try {
		ConfigParser parser;
		WebservConfig config = parser.parseFile(config_file);
		
		if (config.servers.empty()) {
			LOG_ERROR("No server configured in " << config_file);
			return 1;
		}
		
		// Pour l'instant, nous utilisons seulement le premier serveur configuré
		// À l'avenir, nous pourrons lancer plusieurs serveurs en parallèle
		int port = config.servers[0].port;
		LOG_INFO("Loaded configuration with " << config.servers.size() << " server(s)");
		LOG_INFO("Using port " << port << " from configuration");
		
		// Créer et démarrer le serveur
		Server server(port);
		server.start();
	} catch (const std::exception& e) {
		LOG_ERROR("Error: " << e.what());
		return 1;
	}
	
	return 0;
}