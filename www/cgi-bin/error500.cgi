#!/bin/bash

# Script to generate a 500 Internal Server Error
# Generate invalid CGI output (missing content-type header)
echo "This is an invalid CGI response that will cause a 500 error"
exit 1 