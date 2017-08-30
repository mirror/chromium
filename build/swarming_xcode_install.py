#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Run with the following to only effect build23-m7.
  ./client/tools/run_on_bots.py  --swarming touch-swarming.appspot.com --isolate-server touch-isolate.appspot.com --priority 20 --dimension pool Chrome --dimension id build23-m7 ~/work/bling/src/build/swarming_xcode_install.py ~/work/bling/src/build/mac_toolchain.py
"""

import mac_toolchain
import os
import shutil
import subprocess
import tarfile
import tempfile

url = 'gs://chrome-mac-sdk/ios-toolchain-9M214v-1.tgz'
output_dir = '/Applications/Xcode9.0-Beta6.app/'
target_os = 'ios'

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
mac_toolchain.FinalizeUnpack(output_dir, target_os)

