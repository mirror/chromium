#!/usr/bin/perl -wT
use strict;
use warnings FATAL => 'all';
use CGI qw(-oldstyle_urls);
use Encode qw(encode decode);
use File::Basename qw(basename dirname);
use File::Spec::Functions qw(tmpdir);
use utf8;

my $win32 = eval 'use Win32; 1' ? 1 : 0;

open STDERR, '>&STDOUT';  # let the test see die() output
binmode STDOUT, ':encoding(utf-8)';
print "content-type: text/plain; charset=utf-8\n\n";

my $req = CGI->new;
if (uc 'post' ne $req->request_method) {
  die 'Wrong method: ' . $req->request_method;
}
my $filename = decode utf8 => $req->url_param('filename');
$filename =~ /\A([^\0- ]*)\z/s
  or die "untaint failed: $!";
$filename = $1;
my $tempdir = dirname($filename);
my $tmpdir = dirname($tempdir);
my $realtmpdir = $ENV{TMPDIR} || $ENV{TEMP} || $ENV{TMP} || tmpdir();
$realtmpdir =~ /\A([^\0- ]*)\z/s
  or die "untaint failed: $!";
if ($win32) {
  $filename = Win32::GetANSIPathName($filename);
  $tempdir = Win32::GetANSIPathName($tempdir);
  $tmpdir = Win32::GetANSIPathName($tmpdir);
  $realtmpdir = Win32::GetANSIPathName($realtmpdir);
}
my $tempdirbase = basename($tempdir);
if (-f $filename &&
    -d $tempdir &&
    -d $tmpdir &&
    $tmpdir eq $realtmpdir &&
    $tempdirbase =~ /\Aw?cgi\w+\z/) {
  unlink($filename)
    or die(encode utf8 => "unlink $filename: $!");
  rmdir($tempdir)
    or die(encode utf8 => "rmdir $tempdir: $!");
} else {
  die(encode utf8 => "Can't reset $filename");
}
print 'OK';
