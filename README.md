# Webserv

Un serveur HTTP/1.1 conforme et performant, écrit en C++98 pour le projet 42.

## Fonctionnalités

- Serveur HTTP/1.1 conforme
- Configuration similaire à NGINX
- Gestion des méthodes GET, POST et DELETE
- Upload de fichiers
- Exécution de CGI
- Gestion des erreurs et des timeouts
- Multi-port et hôtes virtuels

## Compilation

```bash
make
```

## Utilisation

```bash
./webserv [config_file]
```

Si aucun fichier de configuration n'est spécifié, le serveur utilisera le port 8080 par défaut.

## Configuration

Le fichier de configuration est inspiré de NGINX. Exemple :

```
server {
    listen 8080;
    server_name localhost;
    root ./www;
    client_max_body_size 1M;
    
    location / {
        methods GET POST;
        index index.html;
    }
    
    location /upload {
        methods POST;
        upload_store ./uploads;
    }
}
```

## Structure du projet

- `src/` : Fichiers source
- `include/` : Fichiers d'en-tête
- `www/` : Répertoire racine des fichiers web
- `uploads/` : Répertoire pour les fichiers uploadés

## Auteurs

- Équipe 42 