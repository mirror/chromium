#!/bin/env python
# Copyright 2008, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Ensures trunk/third_party/WebKit is properly modified to reflect forkage.

Moves the forked webkit files so that Visual Studio can't find them.

Example usage:
  ensure_webkit_forked.py MANIFEST 
  ensure_webkit_forked.py unfork
  
  Must be run from third_party/WebKit directory!

"""

import os
import sys
import subprocess

OBSOLETED_EXTENSION = '.obs'

def AddObsoletedExtension(filename):
  return filename + OBSOLETED_EXTENSION

def StripObsoletedExtension(filename):
  if not filename.endswith(OBSOLETED_EXTENSION):
    raise Error('File is not obsoleted. Cannot strip obsoleted extension')
  return filename[:-len(OBSOLETED_EXTENSION)]

def MoveFile(old_filename, new_filename):
  # Revert both files in case we had previously done gvn mv in the other 
  # direction.
  RevertFile(old_filename)
  RevertFile(new_filename)

  AssertOnlyOneFileExists(old_filename, new_filename)

  # If reverting open changes to the file leaves us with the file moved
  # then there's nothing to do.
  if not FileIsInVersionControl(new_filename):
    print 'gvn mv %s %s' % (old_filename, new_filename)
    ExecuteShellCommand('gvn mv %s %s' % (old_filename, new_filename))

def RevertFile(file):
  if FileIsInVersionControl(file):
    print 'gvn revert %s' % file
    ExecuteShellCommand('gvn revert %s' % file)
  
  # If the file is no longer in svn, remove it from the file system.
  if not FileIsInVersionControl(file) and os.path.exists(file):
    print 'Remove unversioned file: ' + file
    os.remove(file)

def GetNormalizedPath(root, filename):
  return os.path.normpath(os.path.join(root, filename))

def AssertOnlyOneFileExists(file, obsoleted_file):
  if FileIsInVersionControl(file) and FileIsInVersionControl(obsoleted_file):
    raise('File should never exist in both obsoleted and unobsoleted forms: ' +
          file)

def FileIsInVersionControl(file):
  # gvn info returns 0 if the file is in svn.
  return ExecuteShellCommand('gvn info %s' % file, True) == 0

def ExecuteShellCommand(command, is_suppress_output=False):
  if is_suppress_output:
    # suppress stdout/stderr as it's too noisy
    return subprocess.call(command, shell=True, stdout=open('nul:', 'w'),
      stderr=open('nul:', 'w'))
  else:
    return subprocess.call(command, shell=True)


def main(manifest):
  """Fork the files. 

  Args:
    manifest: path to manifest file, false to just unfork everything
  """

  already_obsoleted_files = set()
  files_to_obsolete = set()
  
  for root, dirs, files in os.walk('.'):
    for file in files:
      if file.endswith(OBSOLETED_EXTENSION):
        already_obsoleted_files.add(GetNormalizedPath(root, file))

  if manifest != 'unfork':
    for file in open(manifest):
      # Be forgiving of leading/trailing whitespace and ignore commented/empty lines
      file = file.strip()
      if file.startswith('//') or not len(file): continue

      file = os.path.normpath(file)
      obsoleted_file = GetNormalizedPath('.', AddObsoletedExtension(file))


      if obsoleted_file in already_obsoleted_files:
        already_obsoleted_files.remove(obsoleted_file)

      if not FileIsInVersionControl(obsoleted_file):
        MoveFile(file, obsoleted_file)

  # Files leftover are the ones that have been unforked.
  for obsoleted_file in already_obsoleted_files:
    MoveFile(obsoleted_file, StripObsoletedExtension(obsoleted_file))

if '__main__' == __name__:
  main(sys.argv[1])
