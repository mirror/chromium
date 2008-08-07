#!/usr/bin/python
#
# Copyright 2008 Google Inc. All rights reserved.

# Usage: process-ticks.py <logfile>
# Where <logfile> is the log file name (eg, v8.log).

import os, re, sys, tickprocessor, getopt;

class LinuxTickProcessor(tickprocessor.TickProcessor):
  def ParseVMSymbols(self, filename, start, end):
    # Extract symbols and add them to the entries.
    try:
      pipe = os.popen('nm -n %s | c++filt' % filename, 'r')
    except:
      sys.exit("Could not open  " + filename)
    try:
      for line in pipe:
        row = re.match('^([0-9a-fA-F]{8}) . (.*)$', line)
        if row:
          addr = int(row.group(1), 16)
          if addr < start and addr < end - start:
            addr += start
          self.cpp_entries.Insert(addr, tickprocessor.CodeEntry(addr, row.group(2)))
    finally:
      pipe.close()


def Usage():
  print("Usage: linux-tick-processor.py --{js,gc,compiler,other}  logfile-name");
  sys.exit(2)

def Main():
  # parse command line options
  state = None;
  try:
    opts, args = getopt.getopt(sys.argv[1:], "jgco", ["js", "gc", "compiler", "other"])
  except getopt.GetoptError:
    usage()
  # process options.
  for key, value in opts:
    if key in ("-j", "--js"):
      state = 0
    if key in ("-g", "--gc"):
      state = 1
    if key in ("-c", "--compiler"):
      state = 2          
    if key in ("-o", "--other"):
      state = 3
  # do the processing.    
  if len(args) != 1:
      Usage();
  tick_processor = LinuxTickProcessor()
  tick_processor.ProcessLogfile(args[0], state)
  tick_processor.PrintResults()

if __name__ == '__main__':
  Main()
