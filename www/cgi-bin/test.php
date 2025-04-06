<?php
error_reporting(E_ALL);
ini_set('display_errors', 1);

// Log all environment variables
$env_log = "Environment variables:\n";
foreach ($_SERVER as $key => $value) {
    $env_log .= "$key = $value\n";
}
file_put_contents('/tmp/cgi_debug.log', $env_log);

header('Content-Type: application/json');

// Récupérer les données POST
$post_data = array();
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    if (isset($_SERVER['CONTENT_TYPE']) && $_SERVER['CONTENT_TYPE'] === 'application/x-www-form-urlencoded') {
        parse_str(file_get_contents('php://input'), $post_data);
    } else {
        $post_data = $_POST;
    }
}

$response = array(
    'status' => 'success',
    'message' => 'CGI Test Script',
    'server' => array(
        'method' => $_SERVER['REQUEST_METHOD'],
        'query_string' => $_SERVER['QUERY_STRING'],
        'script_name' => $_SERVER['SCRIPT_NAME'],
        'server_software' => $_SERVER['SERVER_SOFTWARE'],
        'gateway_interface' => $_SERVER['GATEWAY_INTERFACE'],
        'redirect_status' => $_SERVER['REDIRECT_STATUS']
    ),
    'environment' => $_SERVER,
    'post_data' => $post_data,
    'raw_input' => file_get_contents('php://input')
);

echo json_encode($response, JSON_PRETTY_PRINT);
?> 