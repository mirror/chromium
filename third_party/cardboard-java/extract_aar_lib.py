#!/usr/bin/env python
#
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Extracts a native library from an Android JAR."""

import os
import sys
import zipfile


def main():
  if len(sys.argv) != 5:
    print 'Usage: %s <android_app_abi> <aar file> <jar file> <so file>' % sys.argv[0]
    sys.exit(1)

  android_app_abi = sys.argv[1]  # e.g. armeabi-v7a
  aar_file = sys.argv[2]  # e.g. path/to/foo.aar
  jar_file = sys.argv[3]
  so_file = sys.argv[4]  # e.g. path/to/libfoo.so

  jar_in_arr = 'classes.jar'
  library_filename = os.path.basename(so_file)
  library_in_aar = os.path.join('jni', android_app_abi, library_filename)

  with zipfile.ZipFile(aar_file, 'r') as archive:
    with open(jar_file, 'wb') as target:
      content = archive.read(jar_in_arr)
      target.write(content)
    with open(so_file, 'wb') as target:
      content = archive.read(library_in_aar)
      target.write(content)


if __name__ == '__main__':
  sys.exit(main())
