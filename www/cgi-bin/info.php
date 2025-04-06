<?php
error_reporting(E_ALL);
ini_set('display_errors', 1);

// Déterminer le format de sortie (HTML ou JSON)
$format = isset($_GET['format']) ? $_GET['format'] : 'html';

if ($format === 'json') {
    header('Content-Type: application/json');
    
    $response = array(
        'server_info' => array(
            'server_software' => $_SERVER['SERVER_SOFTWARE'],
            'server_protocol' => $_SERVER['SERVER_PROTOCOL'],
        ),
        'cgi_info' => array(
            'gateway_interface' => $_SERVER['GATEWAY_INTERFACE'],
            'script_name' => $_SERVER['SCRIPT_NAME'],
            'redirect_status' => $_SERVER['REDIRECT_STATUS']
        ),
        'request_info' => array(
            'request_method' => $_SERVER['REQUEST_METHOD'],
            'remote_addr' => $_SERVER['REMOTE_ADDR'],
            'query_string' => $_SERVER['QUERY_STRING'],
        ),
        'php_info' => array(
            'version' => PHP_VERSION,
            'os' => PHP_OS,
            'sapi' => php_sapi_name()
        )
    );
    
    echo json_encode($response, JSON_PRETTY_PRINT);
} else {
    // Format HTML
    header('Content-Type: text/html');
?>
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Informations Serveur - 42 Webserv</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            line-height: 1.5;
            color: #333;
            max-width: 1200px;
            margin: 0 auto;
            padding: 30px;
            background-color: #f9f9f9;
            text-align: center;
        }
        
        .page-header {
            display: flex;
            flex-direction: column;
            align-items: center;
            margin-bottom: 50px;
        }
        
        .navigation {
            width: 100%;
            display: flex;
            justify-content: flex-start;
            margin-bottom: 20px;
        }
        
        .back-link {
            display: flex;
            align-items: center;
            color: #00babc;
            text-decoration: none;
            font-weight: 500;
            font-size: 1rem;
            background-color: rgba(255, 255, 255, 0.9);
            padding: 8px 16px;
            border-radius: 30px;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.05);
            transition: all 0.2s ease;
        }
        
        .back-link:hover {
            color: #6200ea;
            background-color: #fff;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
            transform: translateY(-2px);
        }
        
        .back-icon {
            margin-right: 8px;
            font-size: 1.1rem;
        }
        
        header {
            background: linear-gradient(135deg, #00babc 0%, #6200ea 100%);
            color: white;
            padding: 25px 40px;
            border-radius: 15px;
            width: 100%;
            max-width: 900px;
            text-align: center;
            box-shadow: 0 4px 20px rgba(0, 188, 188, 0.15);
            position: relative;
            overflow: hidden;
        }
        
        header::before {
            content: '';
            position: absolute;
            top: -10px;
            right: -10px;
            width: 100px;
            height: 100px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 50%;
            z-index: 0;
        }
        
        h1 {
            margin: 0;
            font-size: 2.2rem;
            font-weight: 700;
            position: relative;
            z-index: 1;
        }
        
        header p {
            margin: 10px 0 0;
            font-size: 1.1rem;
            opacity: 0.9;
            position: relative;
            z-index: 1;
        }
        
        .main-content {
            margin-top: 30px;
            display: grid;
            grid-template-columns: 250px 1fr;
            gap: 25px;
            max-width: 900px;
            margin-left: auto;
            margin-right: auto;
        }
        
        @media (max-width: 768px) {
            .main-content {
                grid-template-columns: 1fr;
            }
        }
        
        .card {
            background-color: white;
            border-radius: 15px;
            box-shadow: 0 5px 20px rgba(0, 188, 188, 0.08);
            transition: all 0.3s ease;
        }
        
        .card:hover {
            box-shadow: 0 8px 25px rgba(0, 188, 188, 0.12);
            transform: translateY(-2px);
        }
        
        .nav-card {
            padding: 0;
            overflow: hidden;
        }
        
        .nav-header {
            background: linear-gradient(135deg, #00babc 0%, #008f91 100%);
            color: white;
            padding: 15px 20px;
            font-weight: 600;
            font-size: 1.1rem;
        }
        
        .nav-list {
            list-style: none;
            padding: 0;
            margin: 0;
        }
        
        .nav-item {
            border-bottom: 1px solid #f0f0f0;
        }
        
        .nav-item:last-child {
            border-bottom: none;
        }
        
        .nav-link {
            display: block;
            padding: 14px 20px;
            color: #444;
            text-decoration: none;
            transition: all 0.2s ease;
            font-size: 1rem;
        }
        
        .nav-link:hover, .nav-link.active {
            background-color: #f5f9fa;
            color: #00babc;
            padding-left: 25px;
        }
        
        .nav-link.active {
            font-weight: 500;
            border-left: 3px solid #00babc;
        }
        
        .nav-icon {
            margin-right: 10px;
            width: 24px;
            text-align: center;
            color: #00babc;
            font-size: 1.1rem;
        }
        
        .content-card {
            padding: 25px;
        }
        
        .info-box {
            margin-bottom: 25px;
            break-inside: avoid;
            text-align: left;
        }
        
        .info-box:last-child {
            margin-bottom: 0;
        }
        
        .info-title {
            font-weight: 600;
            color: #333;
            margin-bottom: 15px;
            font-size: 1.2rem;
            display: flex;
            align-items: center;
        }
        
        .info-title-icon {
            margin-right: 10px;
            color: #00babc;
            font-size: 1.3rem;
        }
        
        .key-value-table {
            width: 100%;
            border-collapse: collapse;
            margin-bottom: 15px;
            border-radius: 10px;
            overflow: hidden;
            box-shadow: 0 3px 10px rgba(0, 0, 0, 0.04);
            font-size: 1rem;
        }
        
        .key-value-table tr:nth-child(odd) {
            background-color: #f7f7f7;
        }
        
        .key-value-table tr:hover {
            background-color: #f0f8f8;
        }
        
        .key-value-table td {
            padding: 12px 15px;
            border-bottom: 1px solid #f0f0f0;
        }
        
        .key-value-table tr:last-child td {
            border-bottom: none;
        }
        
        .key-column {
            font-weight: 500;
            color: #555;
            width: 40%;
        }
        
        .value-column {
            font-family: 'SFMono-Regular', Consolas, 'Liberation Mono', Menlo, monospace;
            word-break: break-all;
            font-size: 0.9rem;
        }
        
        .content-tab {
            display: none;
        }
        
        .content-tab.active {
            display: block;
        }
        
        .action-bar {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
        }
        
        .tab-buttons {
            display: flex;
            gap: 10px;
        }
        
        .tab-button {
            background-color: #f0f0f0;
            border: none;
            padding: 8px 16px;
            border-radius: 6px;
            cursor: pointer;
            font-weight: 500;
            color: #555;
            transition: all 0.2s ease;
            font-size: 1rem;
        }
        
        .tab-button:hover {
            background-color: #e5e5e5;
        }
        
        .tab-button.active {
            background-color: #00babc;
            color: white;
        }
        
        .json-link {
            display: inline-flex;
            align-items: center;
            color: #00babc;
            text-decoration: none;
            font-weight: 500;
            font-size: 0.95rem;
            transition: all 0.2s ease;
            padding: 6px 12px;
            border-radius: 6px;
            background-color: rgba(0, 186, 188, 0.08);
        }
        
        .json-link:hover {
            color: #6200ea;
            background-color: rgba(0, 186, 188, 0.15);
        }
        
        .json-icon {
            margin-right: 8px;
            font-size: 1.1rem;
        }
        
        footer {
            text-align: center;
            margin-top: 50px;
            color: #777;
            font-size: 0.9rem;
            padding: 20px;
        }
    </style>
</head>
<body>
    <div class="page-header">
        <div class="navigation">
            <a href="/" class="back-link">
                <span class="back-icon">←</span>Retour à l'accueil
            </a>
        </div>
        <header>
            <h1>Informations Serveur</h1>
            <p>Généré par CGI sur 42 Webserv</p>
        </header>
    </div>

    <div class="main-content">
        <div class="card nav-card">
            <div class="nav-header">Catégories</div>
            <ul class="nav-list">
                <li class="nav-item">
                    <a href="#server" class="nav-link active" data-tab="server-tab">
                        <span class="nav-icon">🖥️</span>Serveur
                    </a>
                </li>
                <li class="nav-item">
                    <a href="#cgi" class="nav-link" data-tab="cgi-tab">
                        <span class="nav-icon">🔄</span>CGI
                    </a>
                </li>
                <li class="nav-item">
                    <a href="#request" class="nav-link" data-tab="request-tab">
                        <span class="nav-icon">📡</span>Requête
                    </a>
                </li>
                <li class="nav-item">
                    <a href="#php" class="nav-link" data-tab="php-tab">
                        <span class="nav-icon">🐘</span>PHP
                    </a>
                </li>
            </ul>
        </div>

        <div class="card content-card">
            <div class="action-bar">
                <div class="tab-buttons">
                    <button class="tab-button active" data-tab="server-tab">Serveur</button>
                    <button class="tab-button" data-tab="cgi-tab">CGI</button>
                    <button class="tab-button" data-tab="request-tab">Requête</button>
                    <button class="tab-button" data-tab="php-tab">PHP</button>
                </div>
                <a href="?format=json" class="json-link">
                    <span class="json-icon">{ }</span>Voir en JSON
                </a>
            </div>

            <!-- Tab Serveur -->
            <div id="server-tab" class="content-tab active">
                <div class="info-box">
                    <div class="info-title">
                        <span class="info-title-icon">🖥️</span>Informations du Serveur
                    </div>
                    <table class="key-value-table">
                        <tr>
                            <td class="key-column">Logiciel Serveur</td>
                            <td class="value-column"><?php echo htmlspecialchars($_SERVER['SERVER_SOFTWARE']); ?></td>
                        </tr>
                        <tr>
                            <td class="key-column">Protocole</td>
                            <td class="value-column"><?php echo htmlspecialchars($_SERVER['SERVER_PROTOCOL']); ?></td>
                        </tr>
                    </table>
                </div>
            </div>

            <!-- Tab CGI -->
            <div id="cgi-tab" class="content-tab">
                <div class="info-box">
                    <div class="info-title">
                        <span class="info-title-icon">🔄</span>Interface CGI
                    </div>
                    <table class="key-value-table">
                        <tr>
                            <td class="key-column">Interface Passerelle</td>
                            <td class="value-column"><?php echo htmlspecialchars($_SERVER['GATEWAY_INTERFACE']); ?></td>
                        </tr>
                        <tr>
                            <td class="key-column">Nom du Script</td>
                            <td class="value-column"><?php echo htmlspecialchars($_SERVER['SCRIPT_NAME']); ?></td>
                        </tr>
                        <tr>
                            <td class="key-column">Statut de Redirection</td>
                            <td class="value-column"><?php echo htmlspecialchars($_SERVER['REDIRECT_STATUS']); ?></td>
                        </tr>
                    </table>
                </div>
            </div>

            <!-- Tab Requête -->
            <div id="request-tab" class="content-tab">
                <div class="info-box">
                    <div class="info-title">
                        <span class="info-title-icon">📡</span>Informations de Requête
                    </div>
                    <table class="key-value-table">
                        <tr>
                            <td class="key-column">Méthode</td>
                            <td class="value-column"><?php echo htmlspecialchars($_SERVER['REQUEST_METHOD']); ?></td>
                        </tr>
                        <tr>
                            <td class="key-column">Adresse Client</td>
                            <td class="value-column"><?php echo htmlspecialchars($_SERVER['REMOTE_ADDR']); ?></td>
                        </tr>
                        <tr>
                            <td class="key-column">Chaîne de Requête</td>
                            <td class="value-column"><?php echo htmlspecialchars($_SERVER['QUERY_STRING']); ?></td>
                        </tr>
                    </table>
                </div>
            </div>

            <!-- Tab PHP -->
            <div id="php-tab" class="content-tab">
                <div class="info-box">
                    <div class="info-title">
                        <span class="info-title-icon">🐘</span>Environnement PHP
                    </div>
                    <table class="key-value-table">
                        <tr>
                            <td class="key-column">Version</td>
                            <td class="value-column"><?php echo PHP_VERSION; ?></td>
                        </tr>
                        <tr>
                            <td class="key-column">Système d'Exploitation</td>
                            <td class="value-column"><?php echo PHP_OS; ?></td>
                        </tr>
                        <tr>
                            <td class="key-column">Interface SAPI</td>
                            <td class="value-column"><?php echo php_sapi_name(); ?></td>
                        </tr>
                    </table>
                </div>
            </div>
        </div>
    </div>

    <footer>
        <p>42 Webserv © 2025</p>
    </footer>

    <script>
        document.addEventListener('DOMContentLoaded', function() {
            // Gestion des onglets
            const tabs = document.querySelectorAll('.tab-button, .nav-link');
            
            tabs.forEach(tab => {
                tab.addEventListener('click', function(e) {
                    e.preventDefault();
                    
                    const targetTabId = this.dataset.tab;
                    
                    // Désactiver tous les onglets
                    document.querySelectorAll('.content-tab').forEach(tabContent => {
                        tabContent.classList.remove('active');
                    });
                    
                    document.querySelectorAll('.tab-button, .nav-link').forEach(tabButton => {
                        tabButton.classList.remove('active');
                    });
                    
                    // Activer l'onglet cible
                    document.getElementById(targetTabId).classList.add('active');
                    
                    // Activer les boutons correspondants
                    document.querySelectorAll(`[data-tab="${targetTabId}"]`).forEach(button => {
                        button.classList.add('active');
                    });
                });
            });
        });
    </script>
</body>
</html>
<?php
}
?> 