#include "Server.hpp"
#include "MultiServerManager.hpp"
#include "utils/Common.hpp"
#include "config/ConfigParser.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[]) {
	std::string config_file = "config.conf";

	if (argc > 1) {
		config_file = argv[1];
	}
	
	// Charger la configuration
	LOG_INFO("Starting webserv with config: " << config_file);
	WebservConfig config;
	try {
		ConfigParser parser;
		config = parser.parseFile(config_file);
	} catch (const std::exception& e) {
		LOG_ERROR("Configuration error: " << e.what());
		return 1;
	}
	
	// No need for detailed log about number of servers
	
	// Initialiser et gérer les serveurs
	MultiServerManager server_manager;
	
	try {
		server_manager.initServers(config);
		server_manager.startServers();
	} catch (const std::exception& e) {
		LOG_ERROR("Server error: " << e.what());
		return 1;
	}
	
	return 0;
}