#!/usr/bin/env python3
import cgi
import json
import sys
import os

# Set proper headers for CGI
print("Content-Type: application/json")
print()  # Empty line to separate headers from body

# Function to convert Celsius to Fahrenheit
def celsius_to_fahrenheit(celsius):
    try:
        celsius = float(celsius)
        fahrenheit = (celsius * 9/5) + 32
        return round(fahrenheit, 2)
    except ValueError:
        return None

# Function to convert Fahrenheit to Celsius
def fahrenheit_to_celsius(fahrenheit):
    try:
        fahrenheit = float(fahrenheit)
        celsius = (fahrenheit - 32) * 5/9
        return round(celsius, 2)
    except ValueError:
        return None

# Get form data
form = cgi.FieldStorage()

# Determine which conversion to perform
direction = form.getvalue("direction", "to_fahrenheit")
temperature = form.getvalue("temperature", "")

result = {
    "status": "success",
    "original": temperature,
    "direction": direction,
    "result": None,
    "unit": "°F" if direction == "to_fahrenheit" else "°C"
}

# Perform the conversion
if direction == "to_fahrenheit":
    result["result"] = celsius_to_fahrenheit(temperature)
else:
    result["result"] = fahrenheit_to_celsius(temperature)

# Check for errors
if result["result"] is None:
    result["status"] = "error"
    result["message"] = "Invalid temperature input"

# Return JSON response
print(json.dumps(result)) 