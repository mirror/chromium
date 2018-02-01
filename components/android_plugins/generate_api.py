#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import sys

REPOSITORY_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..'))
DOCLAVA_DIR = os.path.join(REPOSITORY_ROOT, 'buildtools', 'android', 'doclava')

sys.path.append(os.path.join(REPOSITORY_ROOT, 'build/android/gyp/util'))
import build_utils


def GenerateJavadoc(options):
  javadoc_cmd = [
      'javadoc',
      '-doclet',
      'com.google.doclava.Doclava',
      '-docletpath',
      '%s:%s' % (os.path.join(DOCLAVA_DIR, 'jsilver.jar'),
                 os.path.join(DOCLAVA_DIR, 'doclava.jar')),
      '-api',
      os.path.abspath(options.output),
      '-nodocs',
  ]
  javadoc_cmd += options.files

  build_utils.CheckOutput(
      javadoc_cmd,
      fail_func=lambda ret, stderr: (ret != 0 or not stderr is ''))


def main():
  parser = argparse.ArgumentParser()
  build_utils.AddDepfileOption(parser)
  parser.add_argument('--output', help='Output path')
  parser.add_argument('files', type=str, nargs='+', help='Source files')

  options = parser.parse_args()

  GenerateJavadoc(options)


if __name__ == '__main__':
  sys.exit(main())
