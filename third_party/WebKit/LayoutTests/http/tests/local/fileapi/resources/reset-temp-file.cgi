#!/usr/bin/perl -wT
use strict;
use warnings FATAL => 'all';
use CGI qw(-oldstyle_urls);
use Encode qw(encode decode is_utf8);
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
my $filename_acp = decode utf8 => $req->url_param('filename');
my $filename_acpu = $filename_acp;
if ($win32 && is_utf8($filename_acp)) {
  $filename_acp = encode('cp' . Win32::GetACP(), $filename_acp);
}
$filename_acp =~ /\A([^\0- ]*)\z/s
  or die "untaint failed: $!";
$filename_acp = $1;
if ($win32) {
  $tmpdir = Win32::GetANSIPathName($tmpdir);
}
my $tempdir = dirname($filename_acp);
my $filetmpdir = dirname($tempdir);

my $tempdirbase = basename($tempdir);
if (!-f $filename_acp) {
  die(encode utf8 => "Can't reset $filename_acpu: missing file");
}
if ($filetmpdir ne $tmpdir) {
  die(encode utf8 => "Can't reset $filename_acpu: $filetmpdir is not $tmpdir");
}
if (!-d $tmpdir) {
  die(encode utf8 => "Can't reset $filename_acpu: missing $tmpdir");
}
if (!($tempdirbase =~ /\Aw?cgi\w+\z/)) {
  die(encode utf8 => "Can't reset $filename_acpu: $tempdirbase is not [w]cgi*");
}
if (!-d $tempdir) {
  die(encode utf8 => "Can't reset $filename_acpu: missing $tempdir");
}
unlink($filename_acp)
  or die(encode utf8 => "unlink $filename_acpu: $!");
rmdir($tempdir)
  or die(encode utf8 => "rmdir $tempdir: $!");
print 'OK';
