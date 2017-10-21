#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs a basic telemetry test against a CrOS VM + current chrome."""

import json
import os
import sys


import common


def main_run(args):
  board = os.environ.get('SDK_BOARD', 'amd64-generic')
  assert board in ('amd64-generic', 'betty')

  rc = common.run_command([
      sys.executable,
      os.path.join(common.SRC_DIR, 'third_party', 'chromite', 'bin',
                   'cros_run_vm_test'),
      '--deploy',
      '--build-dir',
      os.path.join(common.SRC_DIR, 'out_%s' % board, 'Release'),
      '--debug',
      '--no-display',
      '--qemu-path',
      os.path.join(common.SRC_DIR, 'third_party', 'qemu', 'linux64',
                   'qemu-system-x86_64')
  ])

  failures = ['failed'] if rc else []

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
