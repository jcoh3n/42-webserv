<?php
// Configuration
header('Content-Type: application/json');

// On utilise un dossier "uploads" simple dans le répertoire www
$upload_dir = '../uploads/';

// Créer le dossier uploads s'il n'existe pas
if (!is_dir($upload_dir)) {
    mkdir($upload_dir, 0755, true);
}

// Récupérer la méthode HTTP
$method = $_SERVER['REQUEST_METHOD'];

// Support pour les requêtes DELETE simulées via POST avec _method=DELETE
if ($method === 'POST' && isset($_POST['_method']) && $_POST['_method'] === 'DELETE') {
    $method = 'DELETE';
}

// Support pour les requêtes DELETE simulées via GET avec _method=DELETE
if ($method === 'GET' && isset($_GET['_method']) && $_GET['_method'] === 'DELETE') {
    $method = 'DELETE';
}

// Fonction simple pour renvoyer une réponse JSON
function send_response($status_code, $status, $message, $data = null) {
    http_response_code($status_code);
    $response = [
        'status' => $status,
        'message' => $message
    ];
    
    if ($data !== null) {
        $response['data'] = $data;
    }
    
    echo json_encode($response);
    exit;
}

// Traiter en fonction de la méthode
switch ($method) {
    case 'POST':
        // Vérifier les paramètres requis
        if (!isset($_POST['action']) || !isset($_POST['filename'])) {
            send_response(400, 'error', 'Paramètres manquants');
        }

        $action = $_POST['action'];
        $filename = $_POST['filename'];
        
        // Nettoyer le nom du fichier (sécurité)
        $filename = basename($filename);
        
        // Action de création de fichier
        if ($action === 'create') {
            $content = isset($_POST['content']) ? $_POST['content'] : '';
            $file_path = $upload_dir . $filename;
            
            // Écrire le contenu dans le fichier
            if (file_put_contents($file_path, $content) !== false) {
                chmod($file_path, 0644); // Permissions de lecture/écriture
                send_response(201, 'success', 'Fichier créé avec succès', [
                    'filename' => $filename,
                    'size' => strlen($content)
                ]);
            } else {
                send_response(500, 'error', 'Erreur lors de la création du fichier');
            }
        } else {
            send_response(400, 'error', 'Action non reconnue');
        }
        break;
        
    case 'DELETE':
        // Pour DELETE, on utilise le paramètre GET ou POST selon la méthode utilisée
        $filename = isset($_GET['filename']) ? $_GET['filename'] : '';
        if (empty($filename) && isset($_POST['filename'])) {
            $filename = $_POST['filename'];
        }
        
        if (empty($filename)) {
            send_response(400, 'error', 'Nom de fichier manquant');
        }
        
        // Nettoyer le nom du fichier (sécurité)
        $filename = basename($filename);
        $file_path = $upload_dir . $filename;
        
        // Vérifier si le fichier existe
        if (!file_exists($file_path)) {
            send_response(404, 'error', 'Fichier non trouvé');
        }
        
        // Supprimer le fichier
        if (unlink($file_path)) {
            send_response(204, 'success', 'Fichier supprimé avec succès');
        } else {
            send_response(500, 'error', 'Erreur lors de la suppression du fichier');
        }
        break;
        
    default:
        send_response(405, 'error', 'Méthode non autorisée');
}
?> 