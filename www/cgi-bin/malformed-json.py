#!/usr/bin/python3

# Ce script génère un JSON malformé pour provoquer une erreur 400

print("Content-Type: application/json")
print()  # Ligne vide séparant les en-têtes du contenu
print("{")  # JSON incomplet et malformé
print("  \"title\": \"Test d'erreur 400\",")
print("  \"message\": \"Ce JSON est délibérément malformé")  # pas de guillemet fermant
print("}") 