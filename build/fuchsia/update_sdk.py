#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Updates the Fuchsia SDK to the given version. Should be used in a 'hooks_os'
entry so that it only runs when .gclient's target_os includes 'fuchsia'."""

import os
import subprocess
import sys

REPOSITORY_ROOT = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..'))
sys.path.append(os.path.join(REPOSITORY_ROOT, 'build'))

import find_depot_tools


def main():
  if len(sys.argv) != 2:
    print >>sys.stderr, 'usage: %s <version>' % sys.argv[0]
    return 1

  version = sys.argv[1]
  root_dir = os.path.join(REPOSITORY_ROOT, 'third_party', 'fuchsia-sdk')
  hash_filename = os.path.join(root_dir, '.hash')

  cmd = [os.path.join(find_depot_tools.DEPOT_TOOLS_PATH, 'cipd'),
         'install', '-root', root_dir, 'fuchsia/sdk/linux-amd64', version]
  subprocess.check_call(cmd)

  with open(hash_filename, 'w') as f:
    f.write(version)

  return 0


if __name__ == '__main__':
  sys.exit(main())
