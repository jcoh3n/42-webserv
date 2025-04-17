<?php
error_reporting(E_ALL);
ini_set('display_errors', 1);

header('Content-Type: application/json');

// Fonction de validation des nombres
function validateNumber($value) {
    if (!is_numeric($value)) {
        throw new Exception("Valeur non numérique: " . htmlspecialchars($value));
    }
    return floatval($value);
}

// Récupérer et valider les données POST ou GET
try {
    $num1 = isset($_REQUEST['num1']) ? validateNumber($_REQUEST['num1']) : 0;
    $num2 = isset($_REQUEST['num2']) ? validateNumber($_REQUEST['num2']) : 0;
    $operation = isset($_REQUEST['operation']) ? $_REQUEST['operation'] : 'add';

    // Valider l'opération
    if (!in_array($operation, array('add', 'subtract', 'multiply', 'divide', 'power'))) {
        throw new Exception("Opération non valide: " . htmlspecialchars($operation));
    }

    // Initialiser la réponse
    $response = array(
        'status' => 'success',
        'timestamp' => date('Y-m-d H:i:s'),
        'input' => array(
            'num1' => $num1,
            'num2' => $num2,
            'operation' => $operation
        ),
        'result' => null,
        'message' => ''
    );

    // Effectuer le calcul
    switch ($operation) {
        case 'add':
            $response['result'] = $num1 + $num2;
            $response['message'] = "$num1 + $num2 = " . $response['result'];
            break;
        case 'subtract':
            $response['result'] = $num1 - $num2;
            $response['message'] = "$num1 - $num2 = " . $response['result'];
            break;
        case 'multiply':
            $response['result'] = $num1 * $num2;
            $response['message'] = "$num1 × $num2 = " . $response['result'];
            break;
        case 'divide':
            if ($num2 == 0) {
                throw new Exception("Division par zéro impossible");
            }
            $response['result'] = $num1 / $num2;
            $response['message'] = "$num1 ÷ $num2 = " . $response['result'];
            break;
        case 'power':
            if ($num2 > 100) {
                throw new Exception("Exposant trop grand (max 100)");
            }
            $response['result'] = pow($num1, $num2);
            $response['message'] = "$num1 ^ $num2 = " . $response['result'];
            break;
    }

    // Vérifier si le résultat est un nombre valide
    if (!is_finite($response['result'])) {
        throw new Exception("Le résultat est infini ou non valide");
    }

} catch (Exception $e) {
    $response['status'] = 'error';
    $response['message'] = $e->getMessage();
}

// Ajouter des informations de performance pour les tests
$response['performance'] = array(
    'execution_time' => number_format((microtime(true) - $_SERVER['REQUEST_TIME_FLOAT']) * 1000, 2) . ' ms'
);

// Désactiver le cache pour les réponses CGI
header('Cache-Control: no-store, no-cache, must-revalidate, max-age=0');
header('Cache-Control: post-check=0, pre-check=0', false);
header('Pragma: no-cache');

echo json_encode($response, JSON_PRETTY_PRINT);
?> 