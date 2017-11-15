#!/usr/bin/perl -wT
# reset-temp-file.cgi - on POST, deletes the file previously written
# by write-temp-file.cgi whose name is supplied in ?[...&]filename=
# and prints OK on success.
#
# This script attempts to redirect any errors to stdout and UTF-8
# encode them to aid diagnostics in the calling test suite; errors are
# distinguished from successful responses by not matching "OK".
#
# NOTE: "ACP" in this file refers to the Windows concept of "ANSI"
# Codepage, a (not actually ANSI-standardized) single- or double-byte
# non-Unicode ASCII-compatible character encoding used for some
# narrow-character Win32 APIs. It should not be confused with UTF-8
# (used for similar purposes on Linux, OS X, and other modern
# POSIX-like systems), nor with the usually-distinct OEM codepage
# used for other narrow-character Win32 APIs (for instance, console
# I/O and some filesystem data storage.)

use strict;
use warnings FATAL => 'all';
use CGI qw(-oldstyle_urls);
use Encode qw(encode decode is_utf8);
use File::Basename qw(basename dirname);
use File::Spec::Functions qw(tmpdir);
use utf8;

my $win32 = eval 'use Win32; 1' ? 1 : 0;

# Win32::GetACP is a recently-added method and not yet available in
# some installations. Fall back to windows-1252 when GetACP is
# missing as that matches the ACP used by our Windows bots.
my $win32_ansi_codepage = 'windows-1252';
eval '$win32_ansi_codepage = "cp" . Win32::GetACP();';

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
  $filename_acp = encode($win32_ansi_codepage, $filename_acp);
}
$filename_acp =~ /\A([^\0- ]*)\z/s
  or die "untaint failed: $!";
$filename_acp = $1;
if ($win32) {
  $tmpdir = Win32::GetANSIPathName($tmpdir);
}
my $tempdir = dirname($filename_acp);
if ($win32) {
  $tmpdir = Win32::GetShortPathName($tmpdir);
  $tempdir = Win32::GetShortPathName($tempdir);
}
my $filetmpdir = dirname($tempdir);
my $tempdirbase = basename($tempdir);
if (!-f $filename_acp) {
  die(encode utf8 => "Can't reset $filename_acpu: missing file");
}
if (uc($filetmpdir) ne uc($tmpdir)) {
  die(encode utf8 => "Can't reset $filename_acpu: $filetmpdir is not $tmpdir");
}
if (!-d $tmpdir) {
  die(encode utf8 => "Can't reset $filename_acpu: missing $tmpdir");
}
if (!($tempdirbase =~ /\Aw?cgi\w+\z/i)) {
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
