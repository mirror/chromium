#!/usr/bin/perl -w

exec("python", $0 =~ s/\.cgi$/.py/r);
