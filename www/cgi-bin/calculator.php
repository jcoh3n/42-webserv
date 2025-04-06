<?php
error_reporting(E_ALL);
ini_set('display_errors', 1);

header('Content-Type: application/json');

// Récupérer les données POST ou GET
$num1 = isset($_REQUEST['num1']) ? floatval($_REQUEST['num1']) : 0;
$num2 = isset($_REQUEST['num2']) ? floatval($_REQUEST['num2']) : 0;
$operation = isset($_REQUEST['operation']) ? $_REQUEST['operation'] : 'add';

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
try {
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
            $response['result'] = pow($num1, $num2);
            $response['message'] = "$num1 ^ $num2 = " . $response['result'];
            break;
        default:
            throw new Exception("Opération non reconnue");
    }
} catch (Exception $e) {
    $response['status'] = 'error';
    $response['message'] = $e->getMessage();
}

// Ajouter des informations de performance pour les tests
$response['performance'] = array(
    'execution_time' => number_format((microtime(true) - $_SERVER['REQUEST_TIME_FLOAT']) * 1000, 2) . ' ms'
);

echo json_encode($response, JSON_PRETTY_PRINT);
?> 