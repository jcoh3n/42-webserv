#!/usr/bin/python3

# Ce script génère délibérément une erreur 501 Not Implemented

# Renvoyer directement un statut 501
print("Status: 501 Not Implemented")
print("Content-Type: text/html")
print()  # Ligne vide séparant les en-têtes du contenu
print("<html><body><h1>501 Not Implemented</h1><p>Cette page ne sera pas affichée car nous avons renvoyé un statut 501</p></body></html>") 