#!/usr/bin/env python
# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" Runs all unittest scripts that are dependent on
      'tools/traffic_annotation/summary/annotations.xml'.
"""

import logging
import os.path
import subprocess
import sys

SCRIPTS = []
# TODO(rhalavati): Update after adding proper unittest.
# 'tools/metrics/histograms/traffic_annotation_histograms_unittest.py'

def main():
  # Get source path assuming this file is in tools/traffic_annotation/scripts.
  src_path = os.path.abspath(
      os.path.join(os.path.dirname(__file__), '..', '..', '..'))
  for script in SCRIPTS:
    args = [os.path.join(src_path, script)]
    if sys.platform == 'win32':
      args.insert(0, "python")
    result = subprocess.call(args)
    if result:
      logging.error("Running '%s' script failed with error code: %i." % (
          script, result))
      return result
  return 0


if __name__ == '__main__':
  sys.exit(main())
