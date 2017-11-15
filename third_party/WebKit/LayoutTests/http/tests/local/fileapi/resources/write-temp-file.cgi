#!/usr/bin/perl -wT
#
# write-temp-file.cgi - creates a file and its temporary subdirectory
#
# The file's desired basename is passed in the filename query parameter.
# The file's desired contents are passed in the data query parameter.
#
# The newly created temporary file will be inside a uniquely-named
# subdirectory to prevent crosstalk between parallel test runs.  Once
# the test is done with the temporary file its
# <pathname_acp> should be passed to delete-temp-file.cgi
# for cleanup.
#
# Must be called using the HTTP POST method.
#
# On success, outputs the following three-line plaintext UTF-8
# response:
#
#   OK
#   <pathname_acp>
#   <pathname>
#
# Where:
# - <pathname_acp> is the full pathname of the just-written
#   temporary file suitable for use with narrow-character APIs after
#   UTF-8 decoding and narrow character-specific reencoding); on
#   non-Windows systems this CGI cannot determine the appropriate
#   locale for the separate web browser and assumes a UTF-8 locale, so
#   this is equal to <pathname> below.
# - <pathname> is the full pathname of the just-written
#   temporary file suitable for use with wide-character Unicode APIs
#   after UTF-8 decoding and UTF-16 reencoding; on non-Windows systems
#   this will equal <pathname_acp> for the reasons outlined
#   above.
#
# Any other output is an error message and indicates failure.
#
# Errors are redirected to stdout and UTF-8 encoded to aid diagnostics
# in the calling test suite.
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

open STDERR, '>&STDOUT';  # let the test see die() output
binmode STDOUT, ':encoding(utf-8)';
autoflush STDOUT 1;
autoflush STDERR 1;
print "content-type: text/plain; charset=utf-8\n\n";

# Find the logical equivalent of /tmp - however extra contortions are
# needed on Win32 with tainting and an unpopulated environment (where
# tmpdir() does not work correctly) to avoid choosing the root
# directory of the active drive by default.
my $local_appdata_temp = tmpdir();
if ($win32) {
  my $local_appdata = Win32::GetFolderPath(Win32::CSIDL_LOCAL_APPDATA());
  if (($local_appdata ne '') && -d "$local_appdata\\Temp") {
    $local_appdata_temp = "$local_appdata\\Temp";
  }
}
# This fallback order works well on fairly sane "vanilla" Win32, OS X
# and Linux Apache configurations with mod_perl.
my $system_tmpdir =
  $ENV{TMPDIR} || $ENV{TEMP} || $ENV{TMP} || $local_appdata_temp;
$system_tmpdir =~ /\A([^\0- ]*)\z/s
  or die "untaint failed: $!";
$system_tmpdir = $1;
if ($win32) {
  $system_tmpdir =
    Win32::GetFullPathName(Win32::GetANSIPathName($system_tmpdir));
}

my $req = CGI->new;
if (uc 'post' ne $req->request_method) {
  die 'Wrong method: ' . $req->request_method;
}
my $basename = decode utf8 => $req->url_param('filename');
$basename =~ /\A([^\0- ]*)\z/s
  or die "untaint failed: $!";
$basename = $1;
my $data = decode utf8 => $req->url_param('data');
# Create a random-named subdirectory of the system temporary directory
# to hold the newly-created test file.
my $temp_cgi_subdir =
  tempdir(($win32 ? 'w' : '') . 'cgiXXXX', DIR => $system_tmpdir);
my $pathname = catfile($temp_cgi_subdir, $basename);
if (dirname($pathname) ne $temp_cgi_subdir) {
  die(encode utf8 => "$pathname not in $temp_cgi_subdir");
}
my $tempfile_contents_write_handle;
my $tempfile;
# $pathname_acpu is a UTF-8-encoded representation of the
# fully-qualified "ACP" file pathname. $pathname_acp is a
# logically-equivalent ACP-encoded bytestring suitable for use in a
# (narrow-character) system call. These are only distinct on Win32.
my $pathname_acp = $pathname;
my $pathname_acpu = $pathname;
if ($win32) {
  # Win32::GetACP is a recently-added method and not yet available in
  # some installations. Fall back to windows-1252 when GetACP is
  # missing as that matches the ACP used by our Windows bots.
  my $win32_ansi_codepage = 'windows-1252';
  eval '$win32_ansi_codepage = "cp" . Win32::GetACP();';

  Win32::CreateFile($pathname)
    or die(encode utf8 => "CreateFile $pathname: $^E");
  $pathname_acp = Win32::GetFullPathName(Win32::GetANSIPathName($pathname));
  $pathname = Win32::GetLongPathName($pathname_acp);
  $pathname_acpu = $pathname_acp;
  if (!is_utf8($pathname_acpu)) {
    $pathname_acpu = decode($win32_ansi_codepage, $pathname_acpu);
  }
  if (!is_utf8($pathname)) {
    $pathname = decode($win32_ansi_codepage, $pathname);
  }
  open($tempfile_contents_write_handle, '>>', $pathname_acp)
    or die(encode utf8 => "open >> $pathname_acpu: $!");
  $tempfile = $pathname_acpu;
} else {
  ($tempfile_contents_write_handle, $tempfile) =
    tempfile(DIR => $temp_cgi_subdir);
}
binmode $tempfile_contents_write_handle, ':encoding(utf-8)';
print $tempfile_contents_write_handle $data;
close $tempfile_contents_write_handle
  or die(encode utf8 => "close $tempfile: $!");
if (!$win32) {
  rename($tempfile, $pathname)
    or die(encode utf8 => "rename $tempfile, $pathname: $!");
}
# This block just verifies that the resulting file in fact exists with
# the expected name and the expected contents. This is actually
# slightly nontrivial since e.g. Mac OS X and Windows both do some
# character normalization in the filesystem which could go wrong, and
# it's much simpler to detect any naming failures here than with a
# mysterious "nothing was actually uploaded" later on in the JS
# test. Likewise it's much easier to detect file contents problems
# (e.g. due to unexpected interplay between Perl "utf-8" native
# strings and the CGI layer) here than later on in the "multipart data
# is somehow wrong" stage.
local $/ = undef;
my $tempfile_verification_read_handle;
open($tempfile_verification_read_handle, '<', $pathname_acp)
  or die(encode utf8 => "open $pathname_acpu: $!");
binmode $tempfile_verification_read_handle, ':encoding(utf-8)';
my $data2 = <$tempfile_verification_read_handle>;
close($tempfile_verification_read_handle)
  or die(encode utf8 => "close $pathname_acpu: $!");
if ($data ne $data2) {
  die(encode utf8 => "Expected $data but got $data2");
}
print "OK\n$pathname_acpu\n$pathname";
