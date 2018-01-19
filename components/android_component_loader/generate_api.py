#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import sys

REPOSITORY_ROOT = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..'))
DOCLAVA_DIR = os.path.join(REPOSITORY_ROOT, 'buildtools', 'android', 'doclava')
DOCLAVA_DIR = '/usr/local/google/code/aosp/out/host/linux-x86/framework/'

sys.path.append(os.path.join(REPOSITORY_ROOT, 'build/android/gyp/util'))
import build_utils

def GenerateJavadoc(options):
  javadoc_cmd = [
    'javadoc',
    '-doclet', 'com.google.doclava.Doclava',
    '-docletpath',
    '%s:%s' % (os.path.join(DOCLAVA_DIR, 'jsilver.jar'),
               os.path.join(DOCLAVA_DIR, 'doclava.jar')),
    '-api', os.path.abspath(options.output),
    '-nodocs',
    'org.chromium.android_component_loader',
  ]
  try:
    build_utils.CheckOutput(javadoc_cmd, cwd=options.input_dir,
        fail_func=lambda ret, stderr: (ret != 0 or not stderr is ''))
  except build_utils.CalledProcessError:
    # build_utils.DeleteDirectory(output_dir)
    raise

def main():
  parser = argparse.ArgumentParser()
  build_utils.AddDepfileOption(parser)
  parser.add_argument('--output', help='Output path')
  parser.add_argument('--input-dir', help='Root of source')

  options = parser.parse_args()

  GenerateJavadoc(options)

if __name__ == '__main__':
  sys.exit(main())
