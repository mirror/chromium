#!/usr/bin/perl -wT
use strict;
use warnings FATAL => 'all';
use CGI qw(-oldstyle_urls);
use Encode qw(decode);
use File::Basename qw(basename dirname);
use File::Spec::Functions qw(tmpdir);
use utf8;

my $win32 = eval 'use Win32::LongPath qw(testL renameL); 1' ? 1 : 0;

binmode STDOUT, ':encoding(utf-8)';
print "content-type: text/plain; charset=utf-8\n\n";

my $req = CGI->new;

if (uc 'post' eq $req->request_method) {
  my $filename = decode utf8 => $req->url_param('filename');
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
        or die "unlinkL $filename: $!";
    } else {
      unlink($filename)
        or die "unlink $filename: $!";
    }
    rmdir($tempdir)
      or die "rmdir $tempdir: $!";
  } else {
    print "Can't reset $filename";
  }
} else {
  print 'Wrong method: ' . $req->request_method . '\n';
}
