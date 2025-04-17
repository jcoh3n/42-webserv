#!/usr/bin/env python

import os
import sys
import cgi

def send_text():
    print "Content-type: text/plain\n\n"
    print "This is plain text content"

def send_html():
    print "Content-type: text/html\n\n"
    print "<html><body><h1>HTML Content</h1><p>This is HTML formatted text</p></body></html>"

def send_json():
    print "Content-type: application/json\n\n"
    print '{"message": "This is JSON data", "status": "success"}'

def main():
    form = cgi.FieldStorage()
    content_type = form.getvalue('type', 'text')
    
    if content_type == 'html':
        send_html()
    elif content_type == 'json':
        send_json()
    else:
        send_text()

if __name__ == "__main__":
    main() 