#!/usr/bin/env python

# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Merges dependency Android manifests into a root manifest."""

import argparse
import os
import sys

from util import build_utils

sys.path.append(os.path.abspath(os.path.join(
  os.path.dirname(__file__), os.pardir)))
from pylib import constants

SDK_TOOLS_LIB_VERSION = '25.3.2'
SDK_TOOLS_LIB_DIR = os.path.join(constants.ANDROID_SDK_ROOT, 'tools', 'lib')

MANIFEST_MERGER_MAIN_CLASS = 'com.android.manifmerger.Merger'
MANIFEST_MERGER_JARS = [
  'common-{version}.jar',
  'manifest-merger-{version}.jar',
  'sdk-common-{version}.jar',
  'sdklib-{version}.jar',
]
MANIFEST_MERGER_CLASSPATH = ':'.join([
  os.path.join(SDK_TOOLS_LIB_DIR, jar.format(version=SDK_TOOLS_LIB_VERSION))
  for jar in MANIFEST_MERGER_JARS
])


def main(argv):
  argv = build_utils.ExpandFileArgs(argv)
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--root-manifest',
                      help='Root manifest which to merge into',
                      required=True)
  parser.add_argument('--output', help='Output manifest path', required=True)
  parser.add_argument('--extras',
                      help='List of additional manifest to merge')
  args = parser.parse_args(argv)

  cmd = [
    'java',
    '-cp', MANIFEST_MERGER_CLASSPATH,
    MANIFEST_MERGER_MAIN_CLASS,
    '--main', args.root_manifest,
    '--out', args.output,
  ]

  extras = build_utils.ParseGnList(args.extras)
  if extras:
    cmd += ['--libs', ':'.join(extras)]

  build_utils.CheckOutput(cmd)


if __name__ == '__main__':
  main(sys.argv[1:])
