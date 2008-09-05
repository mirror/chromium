#!/usr/bin/env python

import sys, os, platform
from os.path import join, abspath, expanduser

# Generate a warning banner if trying to build with older
# versions of Python.
if sys.hexversion < 0x020400F0:
  (major, minor, micro, level, serial) = sys.version_info
  version = str(major) + "." + str(minor) + "." + str(micro)
  print
  print "ERROR: You are trying to build with Python version " + version + "."
  print "It is now required to build with version 2.4 or greater."
  print
  sys.exit(1)

build_dir = abspath('.')

config_files = [
  join(build_dir, 'v8.cfg'),       # Read configuration in build dir
  join(expanduser('~'), '.v8.cfg') # Read configuration in home dir
]

# Read flags from config files, if they exist
flags = ''
for file in config_files:
  if os.path.exists(file):
    text = open(file).read().strip()
    text = text.replace("\n", ' ')
    flags = flags + ' ' + text

v8_dir = join(sys.path[0], '..')

if (abspath(v8_dir) != build_dir):
  # Set the repository to the v8/ directory.
  flags = flags + ' -Y ' + v8_dir
  if os.path.exists(join(v8_dir, 'bin')):
    print 
    print 'Due to the way SCons looks for object files in source'
    print 'repositories, you cannot build outside the source tree'
    print 'when you have a conflicting v8/bin/ directory.  Remove'
    print 'this directory and retry the build.'
    print
    sys.exit(1)

# Build and execute scons on the command-line
scons_path = join(v8_dir, 'third_party', 'scons', 'scons-local-0.97.0d20070918')
os.environ["PYTHONPATH"] = scons_path
start_script = join(v8_dir, 'third_party', 'scons', 'scons.py')

argv_flags = ' '.join(['"%s"' % flag for flag in sys.argv[1:]])
cmdline = sys.executable + ' ' + start_script + flags + ' ' + argv_flags
if os.system(cmdline) != 0: sys.exit(1)
