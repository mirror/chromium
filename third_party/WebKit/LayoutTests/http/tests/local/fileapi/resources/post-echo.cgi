#!/usr/bin/perl -wT
#
# post-echo.cgi - echoes POST body to parent via postMessage

use strict;
use warnings FATAL => 'all';
use utf8;

my $request_method = $ENV{REQUEST_METHOD};
if (uc 'post' ne $request_method) {
  die 'Wrong method: ' . $request_method;
}
my $content_length = $ENV{CONTENT_LENGTH};
if (!($content_length >= 0)) {
  die 'Wrong content-length: ' . $content_length;
}

print "content-type: text/html; charset=utf-8\n\n";
print "<!DOCTYPE html>\n";
print "<meta charset=\"UTF-8\">\n";
print "<body onload=\"parent.postMessage(document.body.innerText, '*')\">\n";
print "<plaintext>";
my $body;
read(STDIN, $body, $content_length);
print $body;
