#ifndef MULTI_SERVER_MANAGER_HPP
# define MULTI_SERVER_MANAGER_HPP

# include "utils/Common.hpp"
# include "Server.hpp"
# include "config/ConfigTypes.hpp"
# include <vector>
# include <map>

class MultiServerManager
{
  private:
    std::vector<Server*> servers;                // Liste des serveurs gérés
    std::map<int, int> port_to_server_index;     // Mapping port -> index de serveur
    
    static MultiServerManager* instance;         // Instance singleton pour le gestionnaire de signaux
    static void signalHandler(int signal);       // Gestionnaire de signal unifié
    
    // Méthodes privées
    void setupSignalHandlers();                  // Configuration des gestionnaires de signal
    
  public:
    MultiServerManager();
    ~MultiServerManager();
    
    // Initialisation et démarrage des serveurs
    void initServers(const WebservConfig& config); // Initialiser les serveurs depuis la configuration
    void startServers();                           // Démarrer tous les serveurs
    void stopServers();                            // Arrêter tous les serveurs
};

#endif 