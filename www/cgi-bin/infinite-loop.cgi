#!/bin/bash

# This script is designed to trigger a 504 Gateway Timeout error

# Output CGI headers first
echo "Content-type: text/html"
echo ""

# Start HTML content
echo "<html><head><title>This will timeout</title></head>"
echo "<body>"
echo "<h1>This request will timeout</h1>"
echo "<p>The script is running but will exceed the execution time limit...</p>"
echo "</body></html>"

# Make sure the output is flushed
exec >&-

# Sleep for longer than the timeout value 
sleep infinity

# This should never be reached
exit 0 