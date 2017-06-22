#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import subprocess
import sys


def cipd(ensure_file, root, service_url):
  return subprocess.call(
      ['cipd', 'ensure',
       '-ensure-file', ensure_file,
       '-root', root
       '-service-url', service_url])


def main():
  parser = argparse.ArgumentParser()

  parser.add_argument(
      '--ensure-file',
      help='Ensure file for dependency.')
  parser.add_argument(
      '--root',
      help='Root directory for dependency.')
  parser.add_argument(\
      '--service-url',
      help='The url of the CIPD service.',
      default='https://chrome-infra-packages.appspot.com')

  args = parser.parse_args()
  cipd(args.ensure_file, args.root, args.service_url)

if __name__ == '__main__':
  sys.exit(main())