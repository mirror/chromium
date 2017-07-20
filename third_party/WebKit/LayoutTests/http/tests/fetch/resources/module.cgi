#!/usr/bin/perl

use IO::Socket;
use CGI;
use Time::HiRes;

my $cgi = new CGI;

$| = 1;

$n = $cgi->param('n');
$step = $cgi->param('step');

autoflush STDOUT 1;

print "Content-Type: text/javascript\n\n";

Time::HiRes::usleep($n * $step * 1000);

for ($i = $n + 1; $i <= 50; ++$i) {
  print "import './module.cgi?n=$i&step=$step';\n";
#  last;
}

print "console.log('I am $n');\n";
