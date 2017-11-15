#!/usr/bin/perl -wT
# write-temp-file.cgi - on POST, writes a file with the supplied
# basename (from ?[...&]filename=) and contents (from ?[...&]data=)
# into a temporary file and returns a three-line plaintext UTF-8
# response on success:
#
#   OK
#   <fullname_acp>
#   <fullname>
#
# Where:
# - <fullname_acp> is the full pathname of the just-written temporary
#   file suitable for use with narrow-character APIs after UTF-8
#   decoding and narrow character-specific reencoding); on non-Windows
#   systems this CGI cannot determine the appropriate locale for the
#   separate web browser and assumes a UTF-8 locale, so this is equal
#   to <fullname> below.
# - <fullname> is the full pathname of the just-written temporary
#   file suitable for use with wide-character Unicode APIs after UTF-8
#   decoding and UTF-16 reencoding; on non-Windows systems this will
#   equal <fullname_acp> for the reasons outlined above.
#
# This script attempts to redirect any errors to stdout and UTF-8
# encode them to aid diagnostics in the calling test suite; errors are
# distinguished from successful responses by not starting with "OK" on
# a line of its own.
#
# The newly created temporary file will be inside a uniquely-named
# subdirectory to prevent crosstalk between multiple test runs. See
# reset-temp-file.cgi for cleanup.
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
use File::Basename qw(dirname);
use File::Spec::Functions qw(catfile tmpdir);
use File::Temp qw(tempdir tempfile);
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
my $filename = decode utf8 => $req->url_param('filename');
$filename =~ /\A([^\0- ]*)\z/s
  or die "untaint failed: $!";
$filename = $1;
my $data = decode utf8 => $req->url_param('data');
if ($win32) {
  $tmpdir = Win32::GetFullPathName(Win32::GetANSIPathName($tmpdir));
}
my $tempdir = tempdir(($win32 ? 'w' : '') . 'cgiXXXX', DIR => $tmpdir);
my $fullname = catfile($tempdir, $filename);
if (dirname($fullname) ne $tempdir) {
  die(encode utf8 => "$fullname not in $tempdir");
}
my $fh;
my $tempfile;
my $fullname_acp = $fullname;
my $fullname_acpu = $fullname_acp;
if ($win32) {
  Win32::CreateFile($fullname)
    or die(encode utf8 => "CreateFile $fullname: $^E");
  $fullname_acp = Win32::GetFullPathName(Win32::GetANSIPathName($fullname));
  $fullname = Win32::GetLongPathName($fullname_acp);
  $fullname_acpu = $fullname_acp;
  if (!is_utf8($fullname_acpu)) {
    $fullname_acpu = decode($win32_ansi_codepage, $fullname_acpu);
  }
  if (!is_utf8($fullname)) {
    $fullname = decode($win32_ansi_codepage, $fullname);
  }
  open($fh, '>>', $fullname_acp)
    or die(encode utf8 => "open >> $fullname_acpu: $!");
  $tempfile = $fullname_acpu;
} else {
  ($fh, $tempfile) = tempfile( DIR => $tempdir );
}
binmode $fh, ':encoding(utf-8)';
print $fh $data;
close $fh
  or die(encode utf8 => "close $tempfile: $!");
if (!$win32) {
  rename($tempfile, $fullname)
    or die(encode utf8 => "rename $tempfile, $fullname: $!");
}
local $/ = undef;
my $fh2;
open($fh2, '<', $fullname_acp)
  or die(encode utf8 => "open $fullname_acpu: $!");
binmode $fh2, ':encoding(utf-8)';
my $data2 = <$fh2>;
close($fh2)
  or die(encode utf8 => "close $fullname_acpu: $!");
if ($data ne $data2) {
  die(encode utf8 => "Expected $data but got $data2");
}
print "OK\n$fullname_acpu\n$fullname";
