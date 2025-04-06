<?php
// Configuration
error_reporting(E_ALL);
ini_set('display_errors', 1);
header('Content-Type: application/json');

// Dossier où les fichiers seront créés (relatif à ce script)
$file_dir = '../';

// Vérifier la méthode HTTP
$method = $_SERVER['REQUEST_METHOD'];

// Fonction pour envoyer une réponse
function sendResponse($code, $status, $message, $data = null) {
    http_response_code($code);
    
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

// Fonction pour valider un nom de fichier
function isValidFilename($filename) {
    // Interdit les tentatives de traversée de répertoire et certains caractères spéciaux
    return !empty($filename) && 
           strpos($filename, '..') === false && 
           strpos($filename, '/') === false && 
           preg_match('/^[a-zA-Z0-9_.-]+$/', $filename);
}

// Traiter la requête selon la méthode
switch ($method) {
    case 'POST':
        $action = $_POST['action'] ?? '';
        
        if ($action === 'create') {
            $filename = $_POST['filename'] ?? '';
            $content = $_POST['content'] ?? '';
            
            if (empty($filename)) {
                sendResponse(400, 'error', 'Nom de fichier manquant');
            }
            
            if (!isValidFilename($filename)) {
                sendResponse(400, 'error', 'Nom de fichier invalide');
            }
            
            $filepath = $file_dir . $filename;
            
            // Tenter d'écrire le fichier
            if (file_put_contents($filepath, $content) !== false) {
                // Définir les permissions pour permettre la lecture et l'écriture
                chmod($filepath, 0644);
                
                sendResponse(201, 'success', 'Fichier créé avec succès', [
                    'filename' => $filename,
                    'size' => strlen($content),
                    'path' => $filepath
                ]);
            } else {
                sendResponse(500, 'error', 'Erreur lors de la création du fichier');
            }
        } else {
            sendResponse(400, 'error', 'Action non reconnue');
        }
        break;
        
    default:
        sendResponse(405, 'error', 'Méthode HTTP non autorisée');
}
?> 