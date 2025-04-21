#!/usr/bin/python3

# Ce script génère délibérément une erreur 408 Request Timeout

# Renvoyer directement un statut 408
print("Status: 408 Request Timeout")
print("Content-Type: text/html")
print()  # Ligne vide séparant les en-têtes du contenu
print("<html><body><h1>408 Request Timeout</h1><p>Cette page ne sera pas affichée car nous avons renvoyé un statut 408</p></body></html>") 