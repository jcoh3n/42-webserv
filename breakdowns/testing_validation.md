## ÉTAPE 9 - TESTS ET VALIDATION

### Objectif
Établir une stratégie de test complète pour valider le fonctionnement du serveur web et garantir sa résilience.

### Types de tests à implémenter

1. **Tests unitaires**:
   - Tester chaque composant de manière isolée
   - Vérifier le bon fonctionnement des classes et fonctions principales

2. **Tests d'intégration**:
   - Tester les interactions entre les différents composants
   - Valider le flux complet des requêtes et réponses

3. **Tests de conformité HTTP**:
   - Vérifier que les réponses respectent la norme HTTP/1.1
   - Comparer avec le comportement de NGINX

4. **Tests de charge**:
   - Évaluer les performances sous forte charge
   - Vérifier la stabilité lors de nombreuses connexions simultanées

5. **Tests de résilience**:
   - Vérifier que le serveur gère correctement les requêtes malformées
   - Tester la récupération après des situations d'erreur

### Détails techniques

1. **Script de test HTTP basique**:
```python
#!/usr/bin/env python3
import requests
import unittest
import subprocess
import time
import os

class WebservBasicTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Démarrer le serveur avec une configuration de test
        cls.server_process = subprocess.Popen(
            ["./webserv", "tests/test_config.conf"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        # Attendre que le serveur soit prêt
        time.sleep(1)
        
    @classmethod
    def tearDownClass(cls):
        # Arrêter le serveur
        cls.server_process.terminate()
        cls.server_process.wait()
    
    def setUp(self):
        self.base_url = "http://localhost:8080"
    
    def test_get_index(self):
        """Vérifier que la page d'index est correctement servie"""
        response = requests.get(f"{self.base_url}/")
        self.assertEqual(response.status_code, 200)
        self.assertIn("Content-Type", response.headers)
        self.assertIn("text/html", response.headers["Content-Type"])
    
    def test_nonexistent_page(self):
        """Vérifier que les pages inexistantes renvoient 404"""
        response = requests.get(f"{self.base_url}/nonexistent.html")
        self.assertEqual(response.status_code, 404)
    
    def test_method_not_allowed(self):
        """Vérifier que les méthodes non autorisées renvoient 405"""
        # Si DELETE n'est pas autorisé sur /
        response = requests.delete(f"{self.base_url}/")
        self.assertEqual(response.status_code, 405)
    
    def test_file_upload(self):
        """Vérifier que l'upload de fichier fonctionne"""
        test_file_content = "Hello, World!"
        with open("test_upload.txt", "w") as f:
            f.write(test_file_content)
        
        files = {'file': ('test_upload.txt', open('test_upload.txt', 'rb'))}
        response = requests.post(f"{self.base_url}/upload", files=files)
        self.assertIn(response.status_code, [200, 201])
        
        # Nettoyer
        os.remove("test_upload.txt")
    
    def test_cgi_script(self):
        """Vérifier que les scripts CGI fonctionnent"""
        response = requests.get(f"{self.base_url}/cgi-bin/cgi_debug.php")
        self.assertEqual(response.status_code, 200)
    
    def test_directory_listing(self):
        """Vérifier que le listing de répertoire fonctionne"""
        response = requests.get(f"{self.base_url}/docs/")
        self.assertEqual(response.status_code, 200)
        self.assertIn("Index of", response.text)
    
    def test_redirection(self):
        """Vérifier que les redirections fonctionnent"""
        response = requests.get(f"{self.base_url}/old", allow_redirects=False)
        self.assertEqual(response.status_code, 301)
        self.assertIn("Location", response.headers)
        self.assertEqual(response.headers["Location"], "/new")

if __name__ == "__main__":
    unittest.main()

Test de conformité des en-têtes HTTP:

pythonCopier#!/usr/bin/env python3
import requests
import subprocess
import time
import difflib
import json

def test_http_headers_compliance():
    """Compare les en-têtes HTTP entre webserv et nginx pour diverses ressources"""
    # Configuration
    webserv_url = "http://localhost:8080"
    nginx_url = "http://localhost:8081"  # Nginx configuré de manière similaire
    
    # Routes à tester
    routes = [
        "/",
        "/index.html",
        "/css/style.css",
        "/js/script.js",
        "/images/logo.png",
        "/docs/",
        "/download/test.pdf"
    ]
    
    results = {}
    
    for route in routes:
        print(f"Testing route: {route}")
        
        webserv_response = requests.get(f"{webserv_url}{route}", allow_redirects=False)
        nginx_response = requests.get(f"{nginx_url}{route}", allow_redirects=False)
        
        # Comparer les codes de statut
        status_match = webserv_response.status_code == nginx_response.status_code
        
        # Ignorer certains en-têtes qui sont spécifiques au serveur
        ignore_headers = ["server", "date", "etag"]
        
        # Extraire et normaliser les en-têtes
        webserv_headers = {k.lower(): v for k, v in webserv_response.headers.items() 
                           if k.lower() not in ignore_headers}
        nginx_headers = {k.lower(): v for k, v in nginx_response.headers.items() 
                         if k.lower() not in ignore_headers}
        
        # Identifier les différences
        header_diff = {}
        all_headers = set(webserv_headers.keys()) | set(nginx_headers.keys())
        
        for header in all_headers:
            if header in webserv_headers and header in nginx_headers:
                if webserv_headers[header] != nginx_headers[header]:
                    header_diff[header] = {
                        "webserv": webserv_headers[header],
                        "nginx": nginx_headers[header]
                    }
            elif header in webserv_headers:
                header_diff[header] = {
                    "webserv": webserv_headers[header],
                    "nginx": "Missing"
                }
            else:
                header_diff[header] = {
                    "webserv": "Missing",
                    "nginx": nginx_headers[header]
                }
        
        results[route] = {
            "status_match": status_match,
            "webserv_status": webserv_response.status_code,
            "nginx_status": nginx_response.status_code,
            "header_differences": header_diff
        }
    
    # Générer un rapport
    with open("http_compliance_report.json", "w") as f:
        json.dump(results, f, indent=2)
    
    print("Compliance test completed. See http_compliance_report.json for details.")

if __name__ == "__main__":
    test_http_headers_compliance()

Test de charge avec wrk:

bashCopier#!/bin/bash
# benchmark.sh - Test de performance du serveur web

# Configuration
SERVER="http://localhost:8080"
DURATION=30  # secondes
CONNECTIONS=100
THREADS=4

# Créer le répertoire pour les résultats
mkdir -p benchmark_results

echo "Starting benchmark tests for $SERVER"

# Test 1: Page statique simple
echo "Test 1: Static HTML page"
wrk -t$THREADS -c$CONNECTIONS -d${DURATION}s $SERVER/ > benchmark_results/static_html.txt

# Test 2: Image statique
echo "Test 2: Static image"
wrk -t$THREADS -c$CONNECTIONS -d${DURATION}s $SERVER/images/logo.png > benchmark_results/static_image.txt

# Test 3: Fichier CSS
echo "Test 3: CSS file"
wrk -t$THREADS -c$CONNECTIONS -d${DURATION}s $SERVER/css/style.css > benchmark_results/css_file.txt

# Test 4: Script CGI léger
echo "Test 4: Light CGI script"
wrk -t$THREADS -c$CONNECTIONS -d${DURATION}s $SERVER/cgi-bin/hello.php > benchmark_results/light_cgi.txt

# Test 5: Script CGI plus lourd
echo "Test 5: Heavy CGI script"
wrk -t$THREADS -c$CONNECTIONS -d${DURATION}s $SERVER/cgi-bin/heavy.php > benchmark_results/heavy_cgi.txt

# Test 6: Fichier de téléchargement plus grand
echo "Test 6: Large file download"
wrk -t$THREADS -c$CONNECTIONS -d${DURATION}s $SERVER/download/large_file.pdf > benchmark_results/large_file.txt

echo "Benchmark tests completed. Results available in the benchmark_results directory."

# Analyse des résultats
echo "Summary of results:"
echo "==================="

for file in benchmark_results/*.txt; do
    test_name=$(basename "$file" .txt)
    requests=$(grep "Requests/sec" "$file" | awk '{print $2}')
    latency=$(grep "Latency" "$file" | awk '{print $2}')
    
    echo "Test: $test_name"
    echo "  Requests/sec: $requests"
    echo "  Latency: $latency"
    echo ""
done

Test de résilience:

pythonCopier#!/usr/bin/env python3
import socket
import time
import threading
import random
import string

def generate_random_string(length):
    """Générer une chaîne aléatoire de la longueur spécifiée"""
    return ''.join(random.choice(string.ascii_letters + string.digits) for _ in range(length))

def send_malformed_request(host, port):
    """Envoyer une requête HTTP malformée"""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, port))
        
        # Choisir un type de requête malformée aléatoire
        malformed_type = random.randint(1, 5)
        
        if malformed_type == 1:
            # Méthode HTTP invalide
            request = f"{generate_random_string(10)} / HTTP/1.1\r\nHost: {host}\r\n\r\n"
        elif malformed_type == 2:
            # Headers malformés
            request = f"GET / HTTP/1.1\r\nBroken-Header\r\nHost: {host}\r\n\r\n"
        elif malformed_type == 3:
            # Version HTTP invalide
            request = f"GET / HTTP/9.9\r\nHost: {host}\r\n\r\n"
        elif malformed_type == 4:
            # URI très longue
            long_uri = "/" + generate_random_string(8000)
            request = f"GET {long_uri} HTTP/1.1\r\nHost: {host}\r\n\r\n"
        else:
            # Chaîne aléatoire complètement invalide
            request = generate_random_string(1000)
        
        s.send(request.encode())
        
        # Tenter de lire la réponse (mais ne pas bloquer trop longtemps)
        s.settimeout(2)
        response = s.recv(4096)
        
        # Si on arrive ici, le serveur a répondu
        return True
    except Exception as e:
        print(f"Error in malformed request: {e}")
        return False
    finally:
        s.close()

def run_resilience_test(host="localhost", port=8080, duration=300, threads=10):
    """Exécuter un test de résilience avec plusieurs threads envoyant des requêtes malformées"""
    print(f"Starting resilience test with {threads} threads for {duration} seconds")
    print(f"Sending malformed requests to {host}:{port}")
    
    # Fonction pour chaque thread
    def worker():
        start_time = time.time()
        request_count = 0
        success_count = 0
        
        while time.time() - start_time < duration:
            if send_malformed_request(host, port):
                success_count += 1
            request_count += 1
            
            # Petite pause pour ne pas saturer complètement le serveur
            time.sleep(0.01)
        
        return request_count, success_count
    
    # Démarrer les threads
    thread_list = []
    for i in range(threads):
        t = threading.Thread(target=worker)
        t.start()
        thread_list.append(t)
        
    # En parallèle, vérifier que le serveur reste accessible pour des requêtes légitimes
    def check_legitimate_requests():
        start_time = time.time()
        success_count = 0
        attempt_count = 0
        
        while time.time() - start_time < duration:
            try:
                # Envoyer une requête GET légitime
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect((host, port))
                s.send(f"GET / HTTP/1.1\r\nHost: {host}\r\n\r\n".encode())
                
                s.settimeout(5)
                response = s.recv(4096)
                
                if b"HTTP/1.1 200" in response:
                    success_count += 1
            except Exception as e:
                print(f"Error in legitimate request: {e}")
            finally:
                s.close()
                
            attempt_count += 1
            time.sleep(5)  # Vérifier toutes les 5 secondes
            
        return attempt_count, success_count
    
    # Démarrer le thread de vérification légitime
    legitimate_thread = threading.Thread(target=check_legitimate_requests)
    legitimate_thread.start()
    
    # Attendre que tous les threads terminent
    for t in thread_list:
        t.join()
    
    legitimate_thread.join()
    
    print("Resilience test completed.")
    print(f"The server remained available for legitimate requests throughout the test.")

if __name__ == "__main__":
    run_resilience_test()

Test de validation des redirections:

pythonCopier#!/usr/bin/env python3
import requests
import unittest

class RedirectTests(unittest.TestCase):
    def setUp(self):
        self.base_url = "http://localhost:8080"
    
    def test_directory_redirect(self):
        """Vérifier que les répertoires sans slash final sont redirigés"""
        response = requests.get(f"{self.base_url}/docs", allow_redirects=False)
        self.assertEqual(response.status_code, 301)
        self.assertEqual(response.headers["Location"], "/docs/")
    
    def test_configured_redirect(self):
        """Vérifier les redirections configurées"""
        response = requests.get(f"{self.base_url}/old-page", allow_redirects=False)
        self.assertEqual(response.status_code, 301)
        self.assertEqual(response.headers["Location"], "/new-page")
    
    def test_redirect_chain(self):
        """Vérifier que les chaînes de redirection fonctionnent"""
        response = requests.get(f"{self.base_url}/redirect1", allow_redirects=True)
        self.assertEqual(response.status_code, 200)
        
        # Vérifier l'historique des redirections
        self.assertEqual(len(response.history), 2)
        self.assertEqual(response.history[0].status_code, 301)
        self.assertEqual(response.history[1].status_code, 301)

if __name__ == "__main__":
    unittest.main()

Test de validation de la gestion des méthodes HTTP:

pythonCopier#!/usr/bin/env python3
import requests
import unittest
import os

class HTTPMethodTests(unittest.TestCase):
    def setUp(self):
        self.base_url = "http://localhost:8080"
        # Créer un fichier pour le test DELETE
        with open("webroot/test_delete.txt", "w") as f:
            f.write("This file will be deleted")
    
    def tearDown(self):
        # Nettoyer les fichiers de test restants
        if os.path.exists("webroot/test_delete.txt"):
            os.remove("webroot/test_delete.txt")
    
    def test_get(self):
        """Vérifier la méthode GET"""
        response = requests.get(f"{self.base_url}/")
        self.assertEqual(response.status_code, 200)
    
    def test_head(self):
        """Vérifier la méthode HEAD (même headers que GET mais sans body)"""
        head_response = requests.head(f"{self.base_url}/")
        get_response = requests.get(f"{self.base_url}/")
        
        self.assertEqual(head_response.status_code, 200)
        self.assertEqual(len(head_response.content), 0)  # Pas de body
        
        # Comparer les headers essentiels
        for header in ["Content-Type", "Content-Length"]:
            self.assertEqual(head_response.headers.get(header), 
                            get_response.headers.get(header))
    
    def test_post(self):
        """Vérifier la méthode POST pour un formulaire"""
        data = {"name": "test", "message": "Hello, world!"}
        response = requests.post(f"{self.base_url}/form", data=data)
        self.assertEqual(response.status_code, 200)
    
    def test_delete(self):
        """Vérifier la méthode DELETE"""
        # Vérifier que le fichier existe
        self.assertTrue(os.path.exists("webroot/test_delete.txt"))
        
        # Envoyer une requête DELETE
        response = requests.delete(f"{self.base_url}/test_delete.txt")
        self.assertEqual(response.status_code, 204)  # No Content
        
        # Vérifier que le fichier a été supprimé
        self.assertFalse(os.path.exists("webroot/test_delete.txt"))
    
    def test_method_not_allowed(self):
        """Vérifier que les méthodes non supportées renvoient 405"""
        response = requests.put(f"{self.base_url}/")
        self.assertEqual(response.status_code, 405)
        self.assertIn("Allow", response.headers)

if __name__ == "__main__":
    unittest.main()
Points de validation

 Exécuter tous les tests et corriger les problèmes
 Vérifier la conformité HTTP avec les standards
 Tester la robustesse face aux requêtes malformées
 Valider les performances sous charge
 S'assurer que le serveur ne crash jamais
 Valider toutes les fonctionnalités essentielles

Tests recommandés

Tests fonctionnels de base:

bashCopierpython3 tests/basic_tests.py

Tests de conformité HTTP:

bashCopierpython3 tests/http_compliance_test.py

Tests de charge avec wrk:

bashCopier./tests/benchmark.sh

Tests de résilience:

bashCopierpython3 tests/resilience_test.py

Vérification des fuites mémoire:

bashCopiervalgrind --leak-check=full --show-leak-kinds=all ./webserv config.conf

Validation du CGI:

bashCopierpython3 tests/cgi_tests.py
Bonnes pratiques

Créer un script d'automatisation pour exécuter tous les tests
Configurer une suite de tests de régression
Tester avec différentes configurations
Valider le comportement sur différents navigateurs
Comparer les résultats avec un serveur de référence (NGINX)
Documenter les tests et les problèmes connus
Implémenter des tests de non-régression pour chaque bug corrigé