#!/usr/bin/perl -wT
use strict;
use warnings FATAL => 'all';
use CGI qw();
use File::Basename qw(dirname);
use File::Spec::Functions qw(catfile tmpdir);
use File::Temp qw(tempdir tempfile);
use utf8;

my $win32 = eval 'use Win32::LongPath qw(renameL); 1' ? 1 : 0;

my $req = CGI->new;

print "content-type: text/plain; charset=utf-8\n\n";

if ('POST' eq $req->request_method) {
  my $filename = $req->url_param('filename');
  $filename =~ /\A([^\0- ]*)\z/s
    or die "untaint failed: $!";
  $filename = $1;
  my $data = $req->url_param('data');
  my $tempdir = tempdir('cgiXXXXXXXXXX', DIR => tmpdir());
  my ($fh, $tempfile) = tempfile( DIR => $tempdir );
  print $fh $data;
  close $fh
    or die "$tempfile: $!";
  my $fullname = catfile($tempdir, $filename);
  if (dirname($fullname) eq $tempdir) {
    if ($win32) {
      renameL($tempfile, $fullname)
        or die "renameL($tempfile, $fullname): $!";
    } else {
      rename($tempfile, $fullname)
        or die "rename($tempfile, $fullname): $!";
    }
    print $fullname;
  } else {
    print "$fullname not in $tempdir";
  }
} else {
  print 'Wrong method: ' . $req->request_method . '\n';
}
