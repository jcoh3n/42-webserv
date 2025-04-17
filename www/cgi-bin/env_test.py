#!/usr/bin/env python

import os;
import sys;

print "Content-type: text/html\n\n";
print "<html><head><title>Environment Test</title></head>";
print "<body><h1>CGI Environment Variables</h1>";
print "<table border='1'>";
print "<tr><th>Variable</th><th>Value</th></tr>";

for param in sorted(os.environ.keys()):
    print "<tr><td>%s</td><td>%s</td></tr>" % (param, os.environ[param]);

print "</table></body></html>"; 