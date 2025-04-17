#!/usr/bin/env python

import sys
import os

def main():
    print "Content-type: text/html\n\n"
    print "<html><head><title>POST Test</title></head>"
    print "<body>"
    print "<h1>POST Data Test</h1>"
    
    # Get content length
    content_length = os.environ.get('CONTENT_LENGTH')
    
    if content_length:
        content_length = int(content_length)
        post_data = sys.stdin.read(content_length)
        print "<p>Received POST data:</p>"
        print "<pre>"
        print post_data
        print "</pre>"
    else:
        print "<p>No POST data received</p>"
        print "<form method='POST' action='/cgi-bin/post_test.py'>"
        print "<textarea name='data'>Enter some test data here</textarea><br>"
        print "<input type='submit' value='Send POST data'>"
        print "</form>"
    
    print "</body></html>"

if __name__ == "__main__":
    main() 