#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import sys


import common


def main_run(args):
  # TODO: Figure out how to get proper test output out of the fuchsia
  # binaries so that we can retry failures, link to logs, etc., but for
  # now we can just do a binary pass/fail.

  rc = common.run_command([
      sys.executable,
      os.path.join(common.SRC_DIR, 'out', args.build_config_fs, 'bin',
                   'run_%s' % args.args[0]),
  ])

  json.dump({
      'valid': True,
      'failures': ['failed'] if rc else [],
  }, args.output)

  return rc


def main_compile_targets(args):
  json.dump([], args.output)


if __name__ == '__main__':
  funcs = {
    'run': main_run,
    'compile_targets': main_compile_targets,
  }
  sys.exit(common.run_script(sys.argv[1:], funcs))
