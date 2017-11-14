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
autoflush STDOUT 1;
autoflush STDERR 1;
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
my $tempdirbase = basename($tempdir);
my $filename_acp = $filename;
if ($win32) {
  $filename_acp = Win32::GetFullPathName(Win32::GetANSIPathName($filename));
}
if (-f $filename_acp &&
    -d $tempdir &&
    -d $tmpdir &&
    $tmpdir eq $realtmpdir &&
    $tempdirbase =~ /\Aw?cgi\w+\z/) {
  unlink($filename_acp)
    or die(encode utf8 => "unlink $filename_acp: $!");
  rmdir($tempdir)
    or die(encode utf8 => "rmdir $tempdir: $!");
} else {
  die(encode utf8 => "Can't reset $filename; not in [w]cgi* in $realtmpdir");
}
print 'OK';
