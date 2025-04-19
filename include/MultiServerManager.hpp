#ifndef MULTI_SERVER_MANAGER_HPP
# define MULTI_SERVER_MANAGER_HPP

# include "utils/Common.hpp"
# include "Server.hpp"
# include "config/ConfigTypes.hpp"
# include <vector>
# include <map>

# define MAX_POLL_SIZE 10240 // Maximum nombre de descripteurs pour poll

/**
 * @brief Gestionnaire centralisé de plusieurs serveurs HTTP
 * 
 * Cette classe gère plusieurs serveurs HTTP simultanément en utilisant
 * un seul appel poll() centralisé pour toutes les opérations d'entrée/sortie.
 */
class MultiServerManager
{
  private:
    std::vector<Server*> servers;                // Liste des serveurs gérés
    std::map<int, int> port_to_server_index;     // Mapping port -> index de serveur
    std::map<int, Server*> fd_to_server;         // Mapping fd -> serveur
    
    struct pollfd* poll_fds;                     // Tableau des descripteurs pour poll
    int nfds;                                    // Nombre de descripteurs actifs
    bool running;                                // État d'exécution des serveurs
    
    static MultiServerManager* instance;         // Instance singleton pour le gestionnaire de signaux
    static void signalHandler(int signal);       // Gestionnaire de signal unifié
    
    // Méthodes privées
    void setupSignalHandlers();                  // Configuration des gestionnaires de signal
    Server* getServerByFd(int fd) const;         // Obtenir le serveur associé à un fd
    void addFdToPoll(int fd, Server* server);    // Ajouter un fd au tableau poll
    void removeFdFromPoll(int fd_index);         // Supprimer un fd du tableau poll
    bool handleEvent(int index);                 // Gère un événement poll, retourne false si le fd a été supprimé
    
  public:
    MultiServerManager();
    ~MultiServerManager();
    
    // Initialisation et démarrage des serveurs
    void initServers(const WebservConfig& config); // Initialiser les serveurs depuis la configuration
    void initializeServers();                      // Nouvelle méthode d'initialisation des serveurs
    void startServers();                           // Démarrer tous les serveurs avec un poll centralisé
    void stopServers();                            // Arrêter tous les serveurs
};

#endif 