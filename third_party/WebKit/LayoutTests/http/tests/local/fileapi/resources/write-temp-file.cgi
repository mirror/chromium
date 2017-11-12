#!/usr/bin/perl -wT
use strict;
use warnings FATAL => 'all';
use CGI qw(-oldstyle_urls);
use Encode qw(encode decode);
use File::Basename qw(dirname);
use File::Spec::Functions qw(catfile tmpdir);
use File::Temp qw(tempdir tempfile);
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
my $data = decode utf8 => $req->url_param('data');
my $tmpdir = $ENV{TMPDIR} || $ENV{TEMP} || $ENV{TMP} || tmpdir();
$tmpdir =~ /\A([^\0- ]*)\z/s
  or die "untaint failed: $!";
my $tempdir = tempdir(($win32 ? 'w' : '') . 'cgiXXXXXXXXXX', DIR => $tmpdir);
if ($win32) {
  $tmpdir = Win32::GetANSIPathName($tmpdir);
  $tempdir = Win32::GetANSIPathName($tempdir);
}
my $fullname = catfile($tempdir, $filename);
if (dirname($fullname) ne $tempdir) {
  die(encode utf8 => "$fullname not in $tempdir");
}
my $fh;
my $tempfile;
my $fullname_acp = $fullname;
if ($win32) {
  Win32::CreateFile($fullname)
    or die(encode utf8 => "Win32::CreateFile $fullname: $^E");
  $fullname_acp = Win32::GetANSIPathName($fullname);
  $tempfile = $fullname_acp;
  open($fh, '>>', $tempfile)
    or die(encode utf8 => "open >> $tempfile: $!");
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
  or die(encode utf8 => "open $fullname_acp: $!");
binmode $fh2, ':encoding(utf-8)';
my $data2 = <$fh2>;
close($fh2)
  or die(encode utf8 => "close $fullname_acp: $!");
if ($data ne $data2) {
  die(encode utf8 => "Expected $data but got $data2");
}
print "OK\n$fullname_acp\n$fullname";
