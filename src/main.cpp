#include "Server.hpp"
#include "MultiServerManager.hpp"
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
		
		LOG_INFO("Loaded configuration with " << config.servers.size() << " server(s)");
		
		// Initialiser et démarrer les serveurs
		MultiServerManager manager;
		manager.initServers(config);
		manager.startServers();
		
		// Assurer un nettoyage propre - cet appel est crucial pour éviter le segfault
		manager.stopServers();
		
	} catch (const std::exception& e) {
		LOG_ERROR("Error: " << e.what());
		return 1;
	}
	
	return 0;
}