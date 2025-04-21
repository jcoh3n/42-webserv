#!/usr/bin/python3

# Ce script génère délibérément une erreur 400 Bad Request

# Renvoyer directement un statut 400
print("Status: 400 Bad Request")
print("Content-Type: text/html")
print()  # Ligne vide séparant les en-têtes du contenu
print("<html><body><h1>400 Bad Request</h1><p>Cette page ne sera pas affichée car nous avons renvoyé un statut 400</p></body></html>") 