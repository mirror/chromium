#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import httplib
import os
import subprocess
import sys

THIS_DIR = os.path.abspath(os.path.dirname(__file__))
GOMA_BIN_DIR = os.path.abspath(os.path.join(THIS_DIR, 'bin'))
GOMA_CTL = os.path.abspath(os.path.join(GOMA_BIN_DIR, 'goma_ctl.py'))


def EnsureGomaCtlIsInstalled():
  # XXX: Windows needs to download
  # https://clients5.google.com/cxx-compiler-service/download/YYYY-MM-DD/
  # instead?
  if os.path.exists(GOMA_CTL):
    return False
  # XXX: We can't |import requests|, so we need to resolve redirects ourselves.
  conn = httplib.HTTPSConnection("clients5.google.com")
  conn.request("GET","/cxx-compiler-service/download/goma_ctl.py")
  res = conn.getresponse()
  if res.status != 200:
    print 'Failed to download goma_ctl script'
    return False
  open(GOMA_CTL, 'wb').write(res.read())
  return True


def EnsureGomaIsUpToDate():
  assert os.path.exists(GOMA_CTL)

  subprocess.call([GOMA_CTL, 'help'])
  subprocess.call([GOMA_CTL, 'help', 'update'])
  subprocess.call([GOMA_CTL, 'update', '--help'])

  # XXX: pick linux / mac via flag instead of from stdin
  subprocess.check_call([GOMA_CTL, 'update'])


def main():
  conn = httplib.HTTPSConnection("clients5.google.com")
  conn.request("HEAD","/cxx-compiler-service/download/goma_ctl.py")
  res = conn.getresponse()
  if res.status == 401:
    print 'Not on google network; exiting'
    return 0

  if not EnsureGomaCtlIsInstalled():
    return 1
  EnsureGomaIsUpToDate()
  return 0

if __name__ == '__main__':
  sys.exit(main())
