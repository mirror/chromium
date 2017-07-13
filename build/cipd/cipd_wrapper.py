#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import subprocess
import sys

ENSURE_FILEPATH = {
  'android': os.path.join(
      os.path.dirname(__file__), 'android', 'android.ensure')
}

def cipd_ensure(root, service_url, target_os):

  ensure_file = ENSURE_FILEPATH[target_os]
  depot_tools_path = os.path.join(root, 'third_party', 'depot_tools')

  cipd_binary = os.path.join(depot_tools_path, 'cipd')
  if sys.platform() in ('cygwin', 'win32'):
    cipd_binary = os.path.join(depot_tools_path, 'cipd.bat')

  return subprocess.call(
      [cipd_binary, 'ensure',
       '-ensure-file', ensure_file,
       '-root', root,
       '-service-url', service_url])


def main():
  parser = argparse.ArgumentParser()

  parser.add_argument(
      '--chromium-root',
      required=True,
      help='Root directory for dependency.')
  parser.add_argument(
      '--service-url',
      help='The url of the CIPD service.',
      default='https://chrome-infra-packages.appspot.com')
  parser.add_argument(
      '--target-os',
      required=True,
      help='Target OS for build.')
  args = parser.parse_args()
  cipd_ensure(args.chromium_root, args.service_url, args.target_os)


if __name__ == '__main__':
  sys.exit(main())