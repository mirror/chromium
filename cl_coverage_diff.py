#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import subprocess

BUILD_PATH = 'out/Coverage-iphonesimulator/'
TEST_TARGET = 'url_unittests'
BASE_BRANCH = 'prototype_cl_code_coverage_diff'
COVERAGE_PROFRAW_IDENTIFIER = 'Coverage data at'

def check_out_master():
  subprocess.check_call(['git', 'checkout', BASE_BRANCH])

def build_test_target():
  subprocess.check_call(['ninja', '-C', BUILD_PATH, '-j', '100', TEST_TARGET])

def run_test_target():
  return subprocess.check_output(['./' + BUILD_PATH + '/iossim', BUILD_PATH + '/' + TEST_TARGET + '.app'])

def main():
  check_out_master()
  build_test_target()
  logs_chracters = run_test_target()
  logs = ''.join(logs_chracters).split('\n')
  coverage_profraw_path = ''
  for log in logs:
    if COVERAGE_PROFRAW_IDENTIFIER in log:
      coverage_profraw_path = log.split(COVERAGE_PROFRAW_IDENTIFIER)[1][1:-1]
      break


if __name__ == '__main__':
  main()
