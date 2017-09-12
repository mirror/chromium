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

version = '9M214v'
url = 'gs://chrome-mac-sdk/ios-toolchain-9M214v-1.tgz'
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

# Download Xcode.
temp_name = tempfile.mktemp(prefix='mac_toolchain')
try:
  subprocess.check_call(['/b/depot_tools/gsutil.py', 'cp', url, temp_name])
  if os.path.exists(output_dir):
    shutil.rmtree(output_dir)
  if not os.path.exists(output_dir):
    os.makedirs(output_dir)
  tarfile.open(mode='r:gz', name=temp_name).extractall(path=output_dir)
finally:
  if os.path.exists(temp_name):
    os.unlink(temp_name)

# Accept license, call runFirstLaunch.
mac_toolchain.FinalizeUnpack(output_dir, 'ios')

if os.path.exists(remove_dir):
  os.unlink(remove_dir)
