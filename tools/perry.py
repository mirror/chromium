#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs all permutations of pairs of tests in a gtest binary to attempt to
detect state leakage between tests."""

import argparse
import os
import multiprocessing
import subprocess
import sys


def GetTestList(path_to_binary):
  """Returns a set of full test names.

  Each test will be of the form "Case.Test". There will be a separate line
  for each combination of Case/Test (there are often multiple tests in each
  case).
  """
  raw_output = subprocess.check_output([path_to_binary, "--gtest_list_tests"])
  input_lines = raw_output.splitlines()

  # The format of the gtest_list_tests output is:
  # "Case1."
  # "  Test1  # <Optional extra stuff>"
  # "  Test2"
  # "Case2."
  # "  Test1"
  case_name = ''  # Includes trailing dot.
  test_set = set()
  for line in input_lines:
    if len(line) > 1:
      if '#' in line:
        line = line[:line.find('#')]
      if line[0] == ' ':
        # Indented means a test in previous case.
        test_set.add(case_name + line.strip())
      else:
        # New test case.
        case_name = line.strip()

  return test_set


def CheckForFailure(pair):
  global g_test_binary
  p = subprocess.Popen(
      [g_test_binary, '--gtest_filter=' + pair[0] + ':' + pair[1]],
      stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
  out, _ = p.communicate()
  if p.returncode != 0:
    return (pair[0], pair[1], out)
  return None


def PrintStatus(i, total, failed):
  status = '%d of %d tested (%d failures)' % (i+1, total, failed)
  print '\r%s%s' % (status, '\x1B[K'),
  sys.stdout.flush()


def main():
  parser = argparse.ArgumentParser(description="Find failing pairs of tests.")
  parser.add_argument('binary', help='Path to gtest binary or wrapper script.')
  args = parser.parse_args()
  print 'Getting test list...'
  all_tests = GetTestList(args.binary)
  global g_test_binary
  g_test_binary = args.binary
  permuted = [(x, y) for x in all_tests for y in all_tests if x != y]

  failed = []
  pool = multiprocessing.Pool()
  total_count = len(permuted)
  for i, result in enumerate(pool.imap_unordered(CheckForFailure, permuted, 1)):
    if result:
      print '\n--gtest_filter=%s:%s failed\n\n%s\n\n' % (
          result[0], result[1], result[2])
      failed.append(result)
    PrintStatus(i, total_count, len(failed))

  pool.terminate()
  pool.join()

  if failed:
    print 'Failed pairs:'
    for f in failed:
      print f

  return 0


if __name__ == '__main__':
  sys.exit(main())
