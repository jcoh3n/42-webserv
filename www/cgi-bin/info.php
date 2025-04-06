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
            'gateway_interface' => $_SERVER['GATEWAY_INTERFACE'],
            'server_protocol' => $_SERVER['SERVER_PROTOCOL'],
            'request_method' => $_SERVER['REQUEST_METHOD'],
            'request_time' => date('Y-m-d H:i:s', $_SERVER['REQUEST_TIME']),
            'remote_addr' => $_SERVER['REMOTE_ADDR'],
            'script_name' => $_SERVER['SCRIPT_NAME'],
            'query_string' => $_SERVER['QUERY_STRING'],
            'redirect_status' => $_SERVER['REDIRECT_STATUS']
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
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif;
            line-height: 1.5;
            color: #333;
            max-width: 800px;
            margin: 0 auto;
            padding: 15px;
            background-color: #f9f9f9;
        }
        header {
            background: linear-gradient(135deg, #00babc 0%, #6200ea 100%);
            color: white;
            padding: 20px;
            border-radius: 12px;
            text-align: center;
            margin-bottom: 15px;
            box-shadow: 0 4px 15px rgba(0, 188, 188, 0.1);
        }
        h1 {
            margin: 0;
            font-size: 2.5rem;
            font-weight: 800;
        }
        .card {
            background-color: white;
            border-radius: 12px;
            padding: 20px;
            margin-bottom: 15px;
            box-shadow: 0 4px 15px rgba(0, 188, 188, 0.08);
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
        }
        th, td {
            padding: 12px 15px;
            text-align: left;
            border-bottom: 1px solid #eee;
        }
        th {
            background-color: #f5f5f5;
            font-weight: 600;
            color: #00babc;
        }
        tr:hover {
            background-color: #f9f9f9;
        }
        .back-link {
            display: inline-block;
            margin-top: 20px;
            color: #00babc;
            text-decoration: none;
            font-weight: 500;
            transition: all 0.2s ease;
        }
        .back-link:hover {
            color: #6200ea;
            text-decoration: underline;
        }
        footer {
            text-align: center;
            margin-top: 20px;
            color: #777;
            font-size: 0.85rem;
            padding: 10px;
        }
    </style>
</head>
<body>
    <header>
        <h1>Informations Serveur</h1>
        <p>Généré par CGI sur 42 Webserv</p>
    </header>

    <div class="card">
        <h2>Informations Serveur</h2>
        <table>
            <tr>
                <th>Variable</th>
                <th>Valeur</th>
            </tr>
            <tr>
                <td>Logiciel Serveur</td>
                <td><?php echo htmlspecialchars($_SERVER['SERVER_SOFTWARE']); ?></td>
            </tr>
            <tr>
                <td>Interface Passerelle (CGI)</td>
                <td><?php echo htmlspecialchars($_SERVER['GATEWAY_INTERFACE']); ?></td>
            </tr>
            <tr>
                <td>Protocole Serveur</td>
                <td><?php echo htmlspecialchars($_SERVER['SERVER_PROTOCOL']); ?></td>
            </tr>
            <tr>
                <td>Méthode de Requête</td>
                <td><?php echo htmlspecialchars($_SERVER['REQUEST_METHOD']); ?></td>
            </tr>
            <tr>
                <td>Heure de Requête</td>
                <td><?php echo date('Y-m-d H:i:s', $_SERVER['REQUEST_TIME']); ?></td>
            </tr>
            <tr>
                <td>Adresse Client</td>
                <td><?php echo htmlspecialchars($_SERVER['REMOTE_ADDR']); ?></td>
            </tr>
            <tr>
                <td>Nom du Script</td>
                <td><?php echo htmlspecialchars($_SERVER['SCRIPT_NAME']); ?></td>
            </tr>
            <tr>
                <td>Chaîne de Requête</td>
                <td><?php echo htmlspecialchars($_SERVER['QUERY_STRING']); ?></td>
            </tr>
            <tr>
                <td>Statut de Redirection</td>
                <td><?php echo htmlspecialchars($_SERVER['REDIRECT_STATUS']); ?></td>
            </tr>
        </table>
        
        <h2>Informations PHP</h2>
        <table>
            <tr>
                <th>Variable</th>
                <th>Valeur</th>
            </tr>
            <tr>
                <td>Version PHP</td>
                <td><?php echo PHP_VERSION; ?></td>
            </tr>
            <tr>
                <td>Système d'Exploitation</td>
                <td><?php echo PHP_OS; ?></td>
            </tr>
            <tr>
                <td>Interface SAPI</td>
                <td><?php echo php_sapi_name(); ?></td>
            </tr>
        </table>
        
        <a href="/" class="back-link">← Retour à la page d'accueil</a>
    </div>

    <footer>
        <p>42 Webserv © 2025</p>
    </footer>
</body>
</html>
<?php
}
?> 