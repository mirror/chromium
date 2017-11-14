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

my $local_appdata_temp = tmpdir();
if ($win32) {
  my $local_appdata = Win32::GetFolderPath(Win32::CSIDL_LOCAL_APPDATA());
  if (($local_appdata ne '') && -d "$local_appdata\\Temp") {
    $local_appdata_temp = "$local_appdata\\Temp";
  }
}
my $tmpdir =
  $ENV{TMPDIR} || $ENV{TEMP} || $ENV{TMP} || $local_appdata_temp;
$tmpdir =~ /\A([^\0- ]*)\z/s
  or die "untaint failed: $!";
$tmpdir = $1;

my $req = CGI->new;
if (uc 'post' ne $req->request_method) {
  die 'Wrong method: ' . $req->request_method;
}
my $filename = decode utf8 => $req->url_param('filename');
$filename =~ /\A([^\0- ]*)\z/s
  or die "untaint failed: $!";
$filename = $1;
my $filename_acp = $filename;
if ($win32) {
  $filename_acp = Win32::GetFullPathName(Win32::GetANSIPathName($filename));
}
my $tempdir = dirname($filename_acp);
my $filetmpdir = dirname($tempdir);

if ($win32) {
  $tmpdir = Win32::GetFullPathName(Win32::GetANSIPathName($tmpdir));
}
my $tempdirbase = basename($tempdir);
if (-f $filename_acp &&
    -d $tempdir &&
    -d $filetmpdir &&
    $filetmpdir eq $tmpdir &&
    $tempdirbase =~ /\Aw?cgi\w+\z/) {
  unlink($filename_acp)
    or die(encode utf8 => "unlink $filename_acp: $!");
  rmdir($tempdir)
    or die(encode utf8 => "rmdir $tempdir: $!");
} else {
  die(encode utf8 => "Can't reset $filename_acp; not in [w]cgi* in $tmpdir");
}
print 'OK';
