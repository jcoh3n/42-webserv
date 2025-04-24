#include "Server.hpp"
#include "MultiServerManager.hpp"
#include "utils/Common.hpp"
#include "config/ConfigParser.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file.conf>" << std::endl;
		return 1;
	}

	std::string config_file = argv[1];
	if (config_file.length() < 5 || config_file.substr(config_file.length() - 5) != ".conf") {
		std::cerr << "Error: Configuration file must have a .conf extension." << std::endl;
		return 1;
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