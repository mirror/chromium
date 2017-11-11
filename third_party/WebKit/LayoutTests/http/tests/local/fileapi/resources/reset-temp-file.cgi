#!/usr/bin/perl -wT
use strict;
use warnings FATAL => 'all';
use CGI qw();
use File::Basename qw(basename dirname);
use File::Spec::Functions qw(tmpdir);
use utf8;

my $win32 = eval 'use Win32::LongPath qw(testL renameL); 1' ? 1 : 0;

my $req = CGI->new;

print "content-type: text/plain; charset=utf-8\n\n";

if ('POST' eq $req->request_method) {
  my $filename = $req->url_param('filename');
  $filename =~ /\A([^\0- ]*)\z/s
    or die "untaint failed: $!";
  $filename = $1;
  my $tempdir = dirname($filename);
  my $tmpdir = dirname($tempdir);
  my $realtmpdir = $ENV{TMPDIR} || $ENV{TEMP} || tmpdir();
  $realtmpdir =~ /\A([^\0- ]*)\z/s
    or die "untaint failed: $!";
  my $tempdirbase = basename($tempdir);
  if (($win32 ? testL('f', $filename) : -f $filename) &&
      -d $tempdir &&
      -d $tmpdir &&
      $tmpdir eq $realtmpdir &&
      $tempdirbase =~ /\Aw?cgi\w+\z/) {
    if ($win32) {
      unlinkL($filename)
        or die "unlinkL($filename): $!";
    } else {
      unlink($filename);
    }
    rmdir($tempdir)
      or die "rmdir($tempdir): $!";
  } else {
    print 'Wrong filename: ' . $filename;
  }
} else {
  print 'Wrong method: ' . $req->request_method . '\n';
}
