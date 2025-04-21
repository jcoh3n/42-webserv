#!/usr/bin/python3

# Ce script génère délibérément une erreur 405 Method Not Allowed

# Renvoyer directement un statut 405
print("Status: 405 Method Not Allowed")
print("Content-Type: text/html")
print("Allow: POST")  # Indiquer quelles méthodes sont autorisées
print()  # Ligne vide séparant les en-têtes du contenu
print("<html><body><h1>405 Method Not Allowed</h1><p>Cette page ne sera pas affichée car nous avons renvoyé un statut 405</p></body></html>") 