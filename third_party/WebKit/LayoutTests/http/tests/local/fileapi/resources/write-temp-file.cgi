#!/usr/bin/perl -wT
use strict;
use warnings FATAL => 'all';
use CGI qw(-oldstyle_urls);
use Encode qw(decode);
use File::Basename qw(dirname);
use File::Spec::Functions qw(catfile tmpdir);
use File::Temp qw(tempdir tempfile);
use utf8;

my $win32 = eval 'use Win32; 1' ? 1 : 0;

binmode STDOUT, ':encoding(utf-8)';
print "content-type: text/plain; charset=utf-8\n\n";

my $req = CGI->new;

if (uc 'post' eq $req->request_method) {
  my $filename = decode utf8 => $req->url_param('filename');
  $filename =~ /\A([^\0- ]*)\z/s
    or die "untaint failed: $!";
  $filename = $1;
  my $data = decode utf8 => $req->url_param('data');
  my $tmpdir = $ENV{TMPDIR} || $ENV{TEMP} || tmpdir();
  $tmpdir =~ /\A([^\0- ]*)\z/s
    or die "untaint failed: $!";
  my $tempdir = tempdir(($win32 ? 'w' : '') . 'cgiXXXXXXXXXX', DIR => $tmpdir);
  if ($win32) {
    $tmpdir = Win32::GetANSIPathName($tmpdir);
    $tempdir = Win32::GetANSIPathName($tempdir);
  }
  my $fullname = catfile($tempdir, $filename);
  if (dirname($fullname) ne $tempdir) {
    die "$fullname not in $tempdir";
  }
  my $fh;
  my $tempfile;
  my $fullname2 = $fullname;
  if ($win32) {
    Win32::CreateFile($fullname)
      or die "Win32::CreateFile $fullname: $^E";
    $fullname2 = Win32::GetANSIPathName($fullname);
    $tempfile = $fullname2;
    open($fh, '>>', $tempfile)
      or die "open >> $tempfile: $!";
  } else {
    ($fh, $tempfile) = tempfile( DIR => $tempdir );
  }
  binmode $fh, ':encoding(utf-8)';
  print $fh $data;
  close $fh
    or die "close $tempfile: $!";
  if (!$win32) {
    rename($tempfile, $fullname)
      or die "rename $tempfile, $fullname: $!";
  }
  local $/ = undef;
  my $fh2;
  open($fh2, '<', $fullname)
    or die "open $fullname: $!";
  binmode $fh2, ':encoding(utf-8)';
  my $data2 = <$fh2>;
  close($fh2)
    or die "close $fullname: $!";
  if ($data eq $data2) {
    print "$fullname2\n$fullname";
  } else {
    print "Expected $data but got $data2";
  }
} else {
  print 'Wrong method: ' . $req->request_method . '\n';
}
