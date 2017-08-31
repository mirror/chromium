#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Package swarming_xcode_install and mac_toolchain and run on swarming bots.
  $  ./build/run_swarming_xcode_install.py  --luci_path ~/work/luci-py
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile


def main():
  parser = argparse.ArgumentParser(
      description='Run swarming_xcode_install on the bots.')
  parser.add_argument('--luci_path', required=True, type=os.path.abspath)
  args = parser.parse_args()

  abspath = os.path.abspath(__file__)
  dname = os.path.dirname(abspath)
  os.chdir(dname)

  tmp_dir = tempfile.mkdtemp(prefix='swarming_xcode')
  try:
    print 'Making isolate.'
    shutil.copyfile('swarming_xcode_install.py',
                    os.path.join(tmp_dir, 'swarming_xcode_install.py'))
    shutil.copyfile('mac_toolchain.py',
                    os.path.join(tmp_dir, 'mac_toolchain.py'))

    luci_client = os.path.join(args.luci_path, 'client')
    cmd = [
      sys.executable, os.path.join(luci_client, 'isolateserver.py'), 'archive',
      '-I', 'touch-isolate.appspot.com', tmp_dir,
    ]
    isolate_hash = subprocess.check_output(cmd).split()[0]

    print 'Running swarming_xcode_install.'
    cmd = [
      sys.executable, os.path.join(luci_client, 'swarming.py'), 'run',
      '--swarming', 'touch-swarming.appspot.com', '--isolate-server',
      'touch-isolate.appspot.com', '--isolated', isolate_hash, '--priority',
      '20', '--dimension', 'pool', 'Chrome', '--dimension', 'id', 'build23-m7',
      '--raw-cmd', '--', 'python', 'swarming_xcode_install.py',
    ]
    subprocess.check_call(cmd)

  finally:
    shutil.rmtree(tmp_dir)
  return 0


if __name__ == '__main__':
  sys.exit(main())
