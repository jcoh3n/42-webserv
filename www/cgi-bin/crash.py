#!/usr/bin/python3

# Ce script se plante délibérément avec une division par zéro

print("Content-Type: text/html")
print()  # Ligne vide séparant les en-têtes du contenu
print("<html><body>")
print("<h1>Ce script va planter</h1>")
print("<p>Une erreur 500 devrait s'afficher à la place de ce contenu.</p>")

# Provoquer une erreur de division par zéro
result = 1 / 0  # Cette ligne provoquera une erreur

print("</body></html>") 