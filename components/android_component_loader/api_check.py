#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import distutils.spawn
import os
import sys

REPOSITORY_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..'))
sys.path.append(os.path.join(REPOSITORY_ROOT, 'build/android/gyp/util'))
import build_utils

DOCLAVA_DIR = os.path.join(REPOSITORY_ROOT, 'buildtools', 'android', 'doclava')


def CheckApi(options):
  # Get the path of the jdk folder by searching for the 'jar' executable. We
  # cannot search for the 'javac' executable because goma provides a custom
  # version of 'javac'. (Copied from build/android/gyp/javac.py)
  jar_path = os.path.realpath(distutils.spawn.find_executable('jar'))
  jdk_dir = os.path.dirname(os.path.dirname(jar_path))

  tools_jar = os.path.join(jdk_dir, 'lib', 'tools.jar')
  jsilver_jar = os.path.join(DOCLAVA_DIR, 'jsilver.jar')
  doclava_jar = os.path.join(DOCLAVA_DIR, 'doclava.jar')

  api_check_cmd = [
      'java',
      '-cp',
      '{}:{}:{}'.format(tools_jar, jsilver_jar, doclava_jar),
      'com.google.doclava.apicheck.ApiCheck',

      # Options copied from aosp build/make/core/tasks/apicheck.mk
      '-hide',
      '2',
      '-hide',
      '3',
      '-hide',
      '4',
      '-hide',
      '5',
      '-hide',
      '6',
      '-hide',
      '24',
      '-hide',
      '25',
      '-hide',
      '26',
      '-hide',
      '27',
      '-error',
      '7',
      '-error',
      '8',
      '-error',
      '9',
      '-error',
      '10',
      '-error',
      '11',
      '-error',
      '12',
      '-error',
      '13',
      '-error',
      '14',
      '-error',
      '15',
      '-error',
      '16',
      '-error',
      '17',
      '-error',
      '18',
      options.old_api,
      options.new_api,
  ]

  build_utils.CheckOutput(
      api_check_cmd,
      fail_func=lambda ret, stderr: (ret != 0 or not stderr is ''))
  if options.stamp:
    build_utils.Touch(options.stamp)


def main():
  parser = argparse.ArgumentParser()
  build_utils.AddDepfileOption(parser)
  parser.add_argument('--old-api', help='Path to old api text file')
  parser.add_argument('--new-api', help='Path to new api text file')
  parser.add_argument('--stamp', help='Path to touch on success')

  options = parser.parse_args()

  if not os.path.isfile(options.old_api):
    print(
        'JAVA STABLE API CHECK WARNING: The specified API file {} does not '
        'exist yet. If this is a new API, copy the initial API definition from'
        ' {} before committing.'.
        format(
            os.path.abspath(options.old_api), os.path.abspath(options.new_api)))
    return

  try:
    CheckApi(options)
  except build_utils.CalledProcessError as err:
    return 'API check error:\n\n{}\n\nOld api: {}\nNew api: {}'.format(
        err.output, os.path.abspath(options.old_api),
        os.path.abspath(options.new_api))


if __name__ == '__main__':
  sys.exit(main())
