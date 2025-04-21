#!/usr/bin/python3

# Ce script génère délibérément une erreur 413 Payload Too Large

# Renvoyer directement un statut 413
print("Status: 413 Payload Too Large")
print("Content-Type: text/html")
print()  # Ligne vide séparant les en-têtes du contenu
print("<html><body><h1>413 Payload Too Large</h1><p>Cette page ne sera pas affichée car nous avons renvoyé un statut 413</p></body></html>") 