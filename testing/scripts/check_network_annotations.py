#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""//testing/scripts wrapper for the network traffic annotations checks."""

import json
import os
import sys


import common


def main_run(args):
  with common.temporary_file() as tempfile_path:
    rc = common.run_command([
        sys.executable,
        os.path.join(common.SRC_DIR, 'tools', 'traffic_annotation', 'scripts',
                     'check_annotations.py'),
        '--build-path',
        os.path.join(args.paths['checkout'], 'out', args.build_config_fs),
        '--json', tempfile_path
    ])

    with open(tempfile_path) as f:
      test_results = json.load(f)
    failures = []

    # Result would be a dictionary with possible 'Error' and/or 'Warning' keys.
    # The expected value of each one is a list of strs.
    for key in test_results:
      for detail in test_results[key]:
        failures += ['%s: %s' % (key, detail)]

  json.dump({
      'valid': True,
      'failures': failures,
  }, args.output)

  return rc


def main_compile_targets(args):
  json.dump(['chrome'], args.output)


if __name__ == '__main__':
  funcs = {
    'run': main_run,
    'compile_targets': main_compile_targets,
  }
  sys.exit(common.run_script(sys.argv[1:], funcs))
