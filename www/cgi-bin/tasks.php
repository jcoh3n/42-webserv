<?php
// Configuration initiale
error_reporting(E_ALL);
ini_set('display_errors', 1);
header('Content-Type: application/json');

// Récupérer la méthode HTTP
$method = $_SERVER['REQUEST_METHOD'];

// Fichier pour stocker les tâches
$tasks_file = '../data/tasks.json';

// S'assurer que le répertoire data existe
if (!is_dir('../data')) {
    mkdir('../data', 0755, true);
}

// Charger les tâches existantes
function loadTasks() {
    global $tasks_file;
    
    if (file_exists($tasks_file)) {
        $content = file_get_contents($tasks_file);
        return json_decode($content, true) ?: [];
    }
    
    return [];
}

// Sauvegarder les tâches
function saveTasks($tasks) {
    global $tasks_file;
    file_put_contents($tasks_file, json_encode($tasks));
}

// Extraire l'ID de la tâche depuis l'URL
function getTaskIdFromUrl() {
    $parts = explode('/', $_SERVER['PATH_INFO'] ?? '');
    return end($parts);
}

// Répondre avec un code d'état et des données JSON
function sendResponse($code, $data = null, $message = '') {
    http_response_code($code);
    
    $response = [
        'status' => $code < 400 ? 'success' : 'error',
        'message' => $message
    ];
    
    if ($data !== null) {
        $response['data'] = $data;
    }
    
    echo json_encode($response);
    exit;
}

// Traiter la requête en fonction de la méthode HTTP
switch ($method) {
    case 'GET':
        // Lister toutes les tâches
        $tasks = loadTasks();
        sendResponse(200, $tasks, 'Tâches récupérées avec succès');
        break;
        
    case 'POST':
        // Ajouter une nouvelle tâche
        $task = $_POST['task'] ?? '';
        
        if (empty($task)) {
            sendResponse(400, null, 'Aucune tâche fournie');
        }
        
        $tasks = loadTasks();
        $tasks[] = $task;
        saveTasks($tasks);
        
        sendResponse(201, ['id' => count($tasks) - 1], 'Tâche ajoutée avec succès');
        break;
        
    case 'DELETE':
        // Supprimer une tâche
        $taskId = getTaskIdFromUrl();
        
        if (!is_numeric($taskId)) {
            sendResponse(400, null, 'ID de tâche invalide');
        }
        
        $tasks = loadTasks();
        $taskId = intval($taskId);
        
        if (!isset($tasks[$taskId])) {
            sendResponse(404, null, 'Tâche non trouvée');
        }
        
        array_splice($tasks, $taskId, 1);
        saveTasks($tasks);
        
        sendResponse(200, null, 'Tâche supprimée avec succès');
        break;
        
    default:
        sendResponse(405, null, 'Méthode non autorisée');
}
?> 