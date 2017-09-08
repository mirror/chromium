#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import sys


import common


def main_run(args):
  running_on_bot = True
  if running_on_bot:
    checkout = args.paths['checkout']
    build_config_fs = args.build_config_fs
    extra_sdk_args = ['--clear-sdk-cache']
    extra_vm_test_args = ['--disable-kvm', '--no-display']
  else:
    checkout = '/usr/local/google/home/achuith/code/chrome/src/'
    build_config_fs = 'Release'
    extra_sdk_args = []
    extra_vm_test_args = []

  board = 'amd64-generic'
  sdk_args = ['cros', 'chrome-sdk', '--board', board, '--download-vm',
              '--internal']
  vm_test_args = ['--', 'cros_run_vm_test', '--deploy', '--build-dir',
                  os.path.join(checkout, 'out_' + board, build_config_fs)]

  run_args = sdk_args + extra_sdk_args + vm_test_args + extra_vm_test_args

  rc = common.run_command(run_args, cwd=checkout)

  failures = ['vm_sanity'] if rc else []

  json.dump({
      'valid': True,
      'failures': failures
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
