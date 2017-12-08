#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess
import sys

THIS_DIR = os.path.abspath(os.path.dirname(__file__))
TARGET_DIR = os.path.abspath(os.path.join(THIS_DIR, 'src'))

src_path = (os.path.normpath(os.path.join(THIS_DIR, '..', '..')))
sys.path.append(os.path.normpath(
    os.path.join(src_path, 'third_party', 'depot_tools')))
import download_from_google_storage

def CanAccessOculusSdkBucket():
  # Checks whether the user has access to gs://chrome-oculus-sdk/.
  gsutil = download_from_google_storage.Gsutil(
      download_from_google_storage.GSUTIL_DEFAULT_PATH, boto_path=None)
  code, _, _ = gsutil.check_call('ls', 'gs://chrome-oculus-sdk/')
  return code == 0

def main():
  # Download libovr source if we have access (build bots and Googlers).
  # For other developers, you must obtain the Oculus SDK yourself.
  if CanAccessOculusSdkBucket():
    gsutil = download_from_google_storage.Gsutil(
      download_from_google_storage.GSUTIL_DEFAULT_PATH, boto_path=None)
    download_from_google_storage.download_from_google_storage(
      directory=True,
      recursive=True,
      input_filename=TARGET_DIR,
      gsutil = gsutil,
      base_url = 'gs://chrome-oculus-sdk',
      num_threads=10,
      force=False,
      output=TARGET_DIR,
      ignore_errors=False,
      sha1_file=False,
      verbose=True,
      auto_platform=False,
      extract=False
      )
  return 0

if __name__ == '__main__':
  sys.exit(main())