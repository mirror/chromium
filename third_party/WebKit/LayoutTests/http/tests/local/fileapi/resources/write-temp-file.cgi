#!/usr/bin/perl -wT
use strict;
use warnings FATAL => 'all';
use CGI qw();
use File::Basename qw(dirname);
use File::Spec::Functions qw(catfile tmpdir);
use File::Temp qw(tempdir tempfile);
use utf8;

my $win32 = eval 'use Win32::LongPath qw(closeL openL renameL); 1' ? 1 : 0;

my $req = CGI->new;

print "content-type: text/plain; charset=utf-8\n\n";

if ('POST' eq $req->request_method) {
  my $filename = $req->url_param('filename');
  $filename =~ /\A([^\0- ]*)\z/s
    or die "untaint failed: $!";
  $filename = $1;
  my $data = $req->url_param('data');
  my $tmpdir = $ENV{TMPDIR} || $ENV{TEMP} || tmpdir();
  $tmpdir =~ /\A([^\0- ]*)\z/s
    or die "untaint failed: $!";
  my $tempdir = tempdir('cgiXXXXXXXXXX', DIR => $tmpdir);
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
    local $/ = undef;
    my $fh2;
    if ($win32) {
      openL(\$fh2, '<', $fullname)
        or die "openL $fullname: $!";
    } else {
      open($fh2, '<', $fullname)
        or die "open $fullname: $!";
    }
    my $data2 = <$fh2>;
    if ($win32) {
      closeL($fh2);
    } else {
      close($fh2);
    }
    if ($data eq $data2) {
      print $fullname;
    } else {
      print "Expected $data but got $data2";
    }
  } else {
    print "$fullname not in $tempdir";
  }
} else {
  print 'Wrong method: ' . $req->request_method . '\n';
}
