#!/usr/bin/perl
use strict;
use warnings;
use POSIX qw(strftime);

my $time = strftime("%H:%M:%S", localtime);
my $date = strftime("%d/%m/%Y", localtime);

print "Content-type: text/html\n\n";
print "<!DOCTYPE html>\n";
print "<html>\n";
print "<head>\n";
print "    <meta charset=\"UTF-8\">\n";
print "    <title>Horloge Perl</title>\n";
print "    <style>\n";
print "        body {\n";
print "            font-family: Arial, sans-serif;\n";
print "            text-align: center;\n";
print "            background-color: #222;\n";
print "            margin: 0;\n";
print "            padding: 0;\n";
print "            color: #fff;\n";
print "            display: flex;\n";
print "            flex-direction: column;\n";
print "            align-items: center;\n";
print "            justify-content: center;\n";
print "            height: 100vh;\n";
print "        }\n";
print "        .clock {\n";
print "            font-size: 7rem;\n";
print "            font-weight: bold;\n";
print "            color: #9C27B0;\n";
print "            margin: 10px 0;\n";
print "            text-shadow: 0 0 15px rgba(156, 39, 176, 0.5);\n";
print "            line-height: 1;\n";
print "        }\n";
print "        .date {\n";
print "            font-size: 1.8rem;\n";
print "            color: #bbb;\n";
print "            margin: 10px 0 30px;\n";
print "            font-weight: 300;\n";
print "        }\n";
print "        .button-container {\n";
print "            display: flex;\n";
print "            gap: 15px;\n";
print "            margin-top: 5px;\n";
print "        }\n";
print "        .btn {\n";
print "            background-color: #333;\n";
print "            color: white;\n";
print "            border: none;\n";
print "            padding: 10px 25px;\n";
print "            border-radius: 4px;\n";
print "            cursor: pointer;\n";
print "            font-size: 1rem;\n";
print "            transition: all 0.2s ease;\n";
print "            font-weight: 400;\n";
print "            text-decoration: none;\n";
print "            display: inline-block;\n";
print "        }\n";
print "        .btn-primary {\n";
print "            background-color: #9C27B0;\n";
print "        }\n";
print "        .btn:hover {\n";
print "            background-color: #444;\n";
print "        }\n";
print "        .btn-primary:hover {\n";
print "            background-color: #7B1FA2;\n";
print "        }\n";
print "    </style>\n";
print "</head>\n";
print "<body>\n";
print "    <div class=\"clock\">$time</div>\n";
print "    <div class=\"date\">$date</div>\n";
print "    <div class=\"button-container\">\n";
print "        <button class=\"btn btn-primary\" onclick=\"location.reload()\">Actualiser</button>\n";
print "        <a href=\"/\" class=\"btn\">Accueil</a>\n";
print "    </div>\n";
print "</body>\n";
print "</html>\n"; 