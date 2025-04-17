#!/usr/bin/env python3
import os
import sys
import cgi
import cgitb
import shutil
from datetime import datetime

# Activer le débogage CGI
cgitb.enable()

# Répertoire où les fichiers seront sauvegardés
UPLOAD_DIR = '../uploads'

def main():
    # Imprimer les en-têtes HTTP
    print("Content-Type: text/html; charset=utf-8")
    print()  # Ligne vide pour séparer les en-têtes du contenu
    
    # Vérifier si le répertoire d'upload existe, sinon le créer
    if not os.path.exists(UPLOAD_DIR):
        try:
            os.makedirs(UPLOAD_DIR)
        except Exception as e:
            print_html_response(False, f"Erreur lors de la création du répertoire d'upload: {str(e)}")
            return
    
    # Vérifier la méthode HTTP
    method = os.environ.get('REQUEST_METHOD', '')
    if method.upper() != 'POST':
        print_html_response(False, "Seule la méthode POST est autorisée pour ce script.")
        return
    
    try:
        # Analyser les données du formulaire
        form = cgi.FieldStorage()
        
        # Vérifier si un fichier a été soumis
        if 'uploaded_file' not in form:
            print_html_response(False, "Aucun fichier n'a été soumis.")
            return
        
        # Récupérer les fichiers
        files = form['uploaded_file']
        
        # Gérer le cas où un seul fichier est envoyé
        if not isinstance(files, list):
            files = [files]
        
        # Récupérer la description (facultative)
        description = form.getvalue('description', '')
        
        # Traiter chaque fichier
        uploaded_files = []
        for fileitem in files:
            if fileitem.filename:
                # Générer un nom de fichier unique pour éviter les collisions
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                filename = f"{timestamp}_{fileitem.filename}"
                file_path = os.path.join(UPLOAD_DIR, filename)
                
                # Sauvegarder le fichier
                with open(file_path, 'wb') as f:
                    shutil.copyfileobj(fileitem.file, f)
                
                uploaded_files.append({
                    'original_name': fileitem.filename,
                    'saved_as': filename,
                    'size': os.path.getsize(file_path)
                })
        
        if uploaded_files:
            print_html_response(True, "Fichiers uploadés avec succès.", uploaded_files, description)
        else:
            print_html_response(False, "Aucun fichier n'a été uploadé.")
            
    except Exception as e:
        print_html_response(False, f"Une erreur est survenue: {str(e)}")

def print_html_response(success, message, files=None, description=None):
    """Affiche une page HTML de réponse"""
    html = f"""<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{'Succès' if success else 'Erreur'} - Upload de fichier</title>
    <style>
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            line-height: 1.6;
            color: #333;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f9f9f9;
        }}
        
        h1, h2 {{
            color: #00babc;
        }}
        
        .card {{
            background-color: #fff;
            border-radius: 12px;
            box-shadow: 0 4px 15px rgba(0, 188, 188, 0.08);
            padding: 25px;
            margin: 20px 0;
        }}
        
        .message {{
            padding: 15px;
            border-radius: 5px;
            margin-bottom: 20px;
        }}
        
        .success {{
            background-color: #e3fcef;
            border-left: 4px solid #00875a;
        }}
        
        .error {{
            background-color: #fff2f0;
            border-left: 4px solid #ff4d4f;
        }}
        
        .file-info {{
            background-color: #f0f0f0;
            padding: 10px;
            border-radius: 5px;
            margin-bottom: 10px;
        }}
        
        .back-link {{
            display: inline-block;
            margin-top: 20px;
            color: #00babc;
            text-decoration: none;
        }}
        
        .back-link:hover {{
            text-decoration: underline;
        }}
    </style>
</head>
<body>
    <div class="card">
        <h1>{'Upload réussi' if success else 'Erreur lors de l\'upload'}</h1>
        
        <div class="message {'success' if success else 'error'}">
            <p>{message}</p>
        </div>
"""
    
    if success and files:
        html += f"""
        <h2>Détails des fichiers uploadés</h2>
"""
        for file in files:
            html += f"""
        <div class="file-info">
            <p><strong>Nom original:</strong> {file['original_name']}</p>
            <p><strong>Enregistré sous:</strong> {file['saved_as']}</p>
            <p><strong>Taille:</strong> {format_size(file['size'])}</p>
        </div>
"""
        
        if description:
            html += f"""
        <h2>Description</h2>
        <p>{description}</p>
"""
    
    html += f"""
        <a href="/pages/file-upload.html" class="back-link">← Retour à la page d'upload</a>
    </div>
</body>
</html>
"""
    print(html)

def format_size(size_bytes):
    """Formate la taille du fichier en unités lisibles"""
    if size_bytes < 1024:
        return f"{size_bytes} octets"
    elif size_bytes < 1024 * 1024:
        return f"{size_bytes / 1024:.2f} Ko"
    elif size_bytes < 1024 * 1024 * 1024:
        return f"{size_bytes / (1024 * 1024):.2f} Mo"
    else:
        return f"{size_bytes / (1024 * 1024 * 1024):.2f} Go"

 