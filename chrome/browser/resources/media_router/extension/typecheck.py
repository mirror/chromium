#!/usr/bin/python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Typecheck JavaScript code if possible.

Java isn't an official requirement for building Chromium on all
platforms, so we need to do something sensible when it's not
available.  Since this script is only used for type checking, we can
just do nothing and report success.
"""

import argparse
import distutils.spawn
import subprocess
import sys

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--out-file')
  parser.add_argument('--compile-script')
  parser.add_argument('source_files', nargs='+')
  args = parser.parse_args()

  if distutils.spawn.find_executable('java'):
    sys.exit(subprocess.call([
        args.compile_script,
        '--out_file', args.out_file,
        '--closure_args', 'dependency_mode=LOOSE', 'checks_only',
        '--',
    ] + args.source_files))

main()
