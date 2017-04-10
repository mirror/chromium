#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Merge driver for the Blink rename merge helper."""

import hashlib
import os
import shutil
import subprocess
import sys


def main():
  if len(sys.argv) < 5:
    print('usage: %s <base> <current> <others> <path in the tree>' %
          sys.argv[0])
    sys.exit(1)

  if 'BLINK_RENAME_RECORDS_PATH' not in os.environ:
    print 'Error: BLINK_RENAME_RECORDS_PATH not set, giving up'
    sys.exit(1)

  base, current, others, file_name_in_tree = sys.argv[1:5]
  shutil.copyfile(others, base)
  print 'File name in tree is %s' % file_name_in_tree
  file_hash = hashlib.sha256(file_name_in_tree).hexdigest()
  shutil.copyfile(
      os.path.join(os.environ['BLINK_RENAME_RECORDS_PATH'], file_hash), current)

  return subprocess.call(['git', 'merge-file', '-Lcurrent', '-Lbase', '-Lother',
                          current, base, others])


if __name__ == '__main__':
  sys.exit(main())
