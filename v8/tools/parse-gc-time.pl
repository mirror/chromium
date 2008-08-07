#!/usr/bin/perl

# take one argument, v8.log, parse the log, and compute the average GC 
# pause time.

use IPC::Open2;

# an array holding scavenge time, in microseconds
my $heapcapacity;  # heap capacity in bytes 
my @scavengetime;
my @markcompacttime;
my $lastgctime;
my $scavengetimemax = 0;
my $markcompacttimemax = 0;

sub die {
  my ($msg) = @_;
  printf $msg;
  exit;
}

sub to_ms {
  my ($s, $ms) = @_;
  return $s * 1000000 + $ms;
}

sub parse_log($) {
  my ($logfile) = @_;
  my $scavenge_begin = 0;
  my $markcompact_begin = 0;
  my $elapsed;

  open (LOGHDR, $logfile);
  while (<LOGHDR>) {
    if ((my $line = $_) =~ /^\(heap-capacity, ([0-9]+)\)$/) {
      $heapcapacity = $1;
    } elsif ($line =~ /^\(scavenge, begin, \(([0-9]+), ([0-9]+)\)\)$/) {
      die("expecting scavenge begin ") if ($scavenge_begin != 0);
      $scavenge_begin = to_ms($1, $2);
    } elsif ($line =~ /^\(scavenge, end, \(([0-9]+), ([0-9]+)\)\)$/) {
      die("expecting scavenge end ") if ($scavenge_begin == 0);
      $lastgctime = to_ms($1, $2);
      $elapsed = $lastgctime - $scavenge_begin;
      $scavengetime[++$#scavengetime] = $elapsed;
      if ($elapsed > $scavengetimemax) { $scavengetimemax = $elapsed; }
      $scavenge_begin = 0;
    } elsif ($line =~ /^\(markcompact, begin, \(([0-9]+), ([0-9]+)\)\)$/) {
      die("expecting markcompact end") if ($markcompact_begin != 0);
      $markcompact_begin = to_ms($1, $2);
    } elsif ($line =~ /^\(markcompact, end, \(([0-9]+), ([0-9]+)\)\)$/) {
      die("expecting markcompact begin") if ($markcompact_begin == 0);
      $lastgctime = to_ms($1, $2);
      $elapsed = $lastgctime - $markcompact_begin;
      $markcompacttime[++$#markcompacttime] = $elapsed;
      if ($elapsed > $markcompacttimemax) { $markcompacttimemax = $elapsed; }
      $markcompact_begin = 0;
    }
  } 
  close(LOGHDR);
}

sub sum {
  my $total = 0;
  for $value (@_) {
    $total += $value;
  }
  return $total;
}

sub average {
  my @values = @_;
  return 0 if (scalar(@values) == 0);
  my $total = sum(@values);
  return $total/scalar(@values) ;
}

my $average_scavenge_time;

parse_log($ARGV[0]);

printf "Heap capacity %d bytes\n", $heapcapacity;
printf "\nScavenge:\n";
printf "  average pause time %d usec\n", average(@scavengetime);
printf "  max pause time %d usec\n", $scavengetimemax;
printf "  frequency : %d\n", scalar(@scavengetime);
printf "\nMarkCompact:\n";
printf "  average pause time %d usec\n", average(@markcompacttime);
printf "  max pause time %d usec\n", $markcompacttimemax;
printf "  frequency : %d\n", scalar(@markcompacttime);

if ($lastgctime > 0) {
  printf "\nTotal run time (approximately) %d usec\n", $lastgctime;
  my $total_gctime = sum(@scavengetime) + sum(@markcompacttime);
  printf "Total gc time %d usec (%%%d)\n", $total_gctime, ($total_gctime*100)/$lastgctime;
}
