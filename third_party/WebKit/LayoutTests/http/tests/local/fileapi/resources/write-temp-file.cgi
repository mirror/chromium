#!/usr/bin/perl -wT
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
    $fullname_acpu = decode('cp' . Win32::GetACP(), $fullname_acpu);
  }
  if (!is_utf8($fullname)) {
    $fullname = decode('cp' . Win32::GetACP(), $fullname);
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
