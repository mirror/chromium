#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to download lld from google storage."""

import os
import re
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
CHROME_SRC = os.path.abspath(os.path.join(SCRIPT_DIR, '..', '..', '..'))

sys.path.insert(0, os.path.join(CHROME_SRC, 'build'))
import find_depot_tools


DEPOT_PATH = find_depot_tools.add_depot_tools_to_path()
GSUTIL_PATH = os.path.join(DEPOT_PATH, 'gsutil.py')

LLVM_BUILD_PATH = os.path.join(CHROME_SRC, 'third_party', 'llvm-build',
                               'Release+Asserts')
LLD_PATH = os.path.join(LLVM_BUILD_PATH, 'bin', 'lld')
LD_LLD_PATH = os.path.join(LLVM_BUILD_PATH, 'bin', 'ld.lld')
LLD_LINK_PATH = os.path.join(LLVM_BUILD_PATH, 'bin', 'lld-link')
CLANG_UPDATE_PY = os.path.join(CHROME_SRC, 'tools', 'clang', 'scripts',
                               'update.py')
CLANG_REVISION = os.popen(CLANG_UPDATE_PY + ' --print-revision').read().rstrip()

CLANG_BUCKET = 'gs://chromium-browser-clang/Mac'


def AlreadyUpToDate():
  if not os.path.exists(LLD_PATH):
    return False
  # lld-link seems to have no flag to query the version; go to ld.lld instead.
  # TODO(thakis): Remove ld.lld workaround if https://llvm.org/PR34955 is fixed.
  lld_rev = subprocess.check_output([LD_LLD_PATH, '--version'])
  return re.match(r'LLD.*\(trunk (\d+)\)', lld_rev).group(1) != CLANG_REVISION


def main():
  if AlreadyUpToDate():
    return 0

  targz_name = 'lld-%s.tgz' % CLANG_REVISION
  remote_path = '%s/%s' % (CLANG_BUCKET, targz_name)

  os.chdir(LLVM_BUILD_PATH)

  subprocess.check_call(['python', GSUTIL_PATH,
                         'cp', remote_path, targz_name])
  subprocess.check_call(['tar', 'xzf', targz_name])
  os.remove(targz_name)

  # TODO(thakis): Remove this once the lld tgz includes the symlink.
  if os.path.exists(LLD_LINK_PATH):
    os.remove(LLD_LINK_PATH)
    os.remove(LD_LLD_PATH)
  os.symlink('lld', LLD_LINK_PATH)
  os.symlink('lld', LD_LLD_PATH)
  return 0

if __name__ == '__main__':
  sys.exit(main())
