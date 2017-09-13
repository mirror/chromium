#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Script used to install Xcode on the swarming bots.
"""

import mac_toolchain
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile

version = '9A235'
url = 'gs://chrome-mac-sdk/ios-toolchain-9A235-1.tgz'
remove_dir = '/Applications/Xcode9.0-Beta4.app/'
output_dir = '/Applications/Xcode9.0.app/'

# Check if it's already installed.
if os.path.exists(output_dir):
  env = os.environ.copy()
  env['DEVELOPER_DIR'] = output_dir
  cmd = ['xcodebuild', '-version']
  found_version = \
      subprocess.Popen(cmd, env=env, stdout=subprocess.PIPE).communicate()[0]
  if version in found_version:
    print "Xcode %s already installed" % version
    sys.exit(0)

# Confirm old dir is there first
if not os.path.exists(remove_dir):
  print "Failing early since %s isn't there." % remove_dir
  sys.exit(1)

# Download Xcode.
with tempfile.NamedTemporaryFile() as temp:
  env = os.environ.copy()
  env['PATH'] += ":/b/depot_tools"
  subprocess.check_call(['gsutil.py', 'cp', url, temp.name], env=env)
  if os.path.exists(output_dir):
    shutil.rmtree(output_dir)
  if not os.path.exists(output_dir):
    os.makedirs(output_dir)
  tarfile.open(mode='r:gz', name=temp.name).extractall(path=output_dir)

# Accept license, call runFirstLaunch.
mac_toolchain.FinalizeUnpack(output_dir, 'ios')

# Set new Xcode as default.
subprocess.check_call(['sudo', '/usr/bin/xcode-select', '-s', output_dir])

if os.path.exists(remove_dir):
  shutil.rmtree(remove_dir)
