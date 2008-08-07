#!/usr/bin/env python

# Usage: process-ticks.py <binary> <logfile>
#
# Where <binary> is the binary program name (eg, v8_shell.exe) and
# <logfile> is the log file name (eg, v8.log).
#
# This tick processor expects to find a map file for the binary named
# binary.map if the binary is named binary.exe. The tick processor
# only works for statically linked executables - no information about
# shared libraries is logged from v8 on Windows.

import os, re, sys, tickprocessor, getopt

class WindowsTickProcessor(tickprocessor.TickProcessor):
  # Very simple unmangling of C++ names.  Does not handle arguments and
  # template arguments.
  #
  # The mangled names have the form:
  #
  #   ?LookupInDescriptor@JSObject@internal@v8@@...arguments info...
  #
  def Unmangle(self, name):
    # Name is mangled if it starts with a question mark.
    is_mangled = re.match("^\?(.*)", name)
    if is_mangled:
      substrings = is_mangled.group(1).split('@')
      try:
        # The function name is terminated by two @s in a row.  Find the
        # substrings that are part of the function name.
        index = substrings.index('')
        substrings = substrings[0:index]
      except ValueError:
        # If we did not find two @s in a row, the mangled name is not in
        # the format we expect and we give up.
        return name
      substrings.reverse()
      function_name = "::".join(substrings)
      return function_name
    return name

  # Parse map file and add symbol information to the cpp-entries table.
  def ParseMapFile(self, filename):
    # Locate map file.
    has_dot = re.match('^([a-zA-F0-9_-]*)[\.]?.*$', filename)
    if has_dot:
      map_file_name = has_dot.group(1) + '.map'
      try:
        map_file = open(map_file_name, 'rb')
      except IOError:
        sys.exit("Could not open map file: " + map_file_name)
    else:
      sys.exit("Could not find map file for executable: " + filename)
    try:
      max_addr = 0
      min_addr = 2**30
      # Process map file and search for function entries.
      row_regexp = re.compile(' 0001:[0-9a-fA-F]{8}\s*([_\?@$0-9a-zA-Z]*)\s*([0-9a-fA-F]{8}).*')
      for line in map_file:
        row = re.match(row_regexp, line)
        if row:
          addr = int(row.group(2), 16)
          if addr > max_addr:
            max_addr = addr
          if addr < min_addr:
            min_addr = addr
          mangled_name = row.group(1)
          name = self.Unmangle(mangled_name)
          self.cpp_entries.Insert(addr, tickprocessor.CodeEntry(addr, name));
      i = min_addr
      # Mark the pages for which there are functions in the map file.
      while i < max_addr:
        page = i >> 12
        self.vm_extent[page] = 1
        i += 4096
    finally:
      map_file.close()

def Usage():
  print("Usage: windows-tick-processor.py binary logfile-name");
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
  if len(args) != 2:
      Usage();
  tickprocessor = WindowsTickProcessor()
  tickprocessor.ParseMapFile(args[0])
  tickprocessor.ProcessLogfile(args[1], state)
  tickprocessor.PrintResults()

if __name__ == '__main__':
  Main()
