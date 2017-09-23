#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import time
import unittest


TEST_SEPARATOR = '.'

def _FullResults(suite, result, metadata):
  """Convert the unittest results to the Chromium JSON test result format.

  This matches run-webkit-tests (the layout tests) and the flakiness dashboard.
  """

  full_results = {}
  full_results['interrupted'] = False
  full_results['path_delimiter'] = TEST_SEPARATOR
  full_results['version'] = 3
  full_results['seconds_since_epoch'] = time.time()
  for md in metadata:
    key, val = md.split('=', 1)
    full_results[key] = val

  all_test_names = _AllTestNames(suite)
  failed_test_names = _FailedTestNames(result)

  full_results['num_failures_by_type'] = {
      'FAIL': len(failed_test_names),
      'PASS': len(all_test_names) - len(failed_test_names),
  }

  full_results['tests'] = {}

  for test_name in all_test_names:
    value = {}
    value['expected'] = 'PASS'
    if test_name in failed_test_names:
      value['actual'] = 'FAIL'
      value['is_unexpected'] = True
    else:
      value['actual'] = 'PASS'
    _AddPathToTrie(full_results['tests'], test_name, value)

  return full_results


def _AllTestNames(suite):
  test_names = []
  # _tests is protected  pylint: disable=W0212
  for test in suite._tests:
    if isinstance(test, unittest.suite.TestSuite):
      test_names.extend(_AllTestNames(test))
    else:
      test_names.append(test.id())
  return test_names


def _FailedTestNames(result):
  return set(test.id() for test, _ in result.failures + result.errors)


def _AddPathToTrie(trie, path, value):
  if TEST_SEPARATOR not in path:
    trie[path] = value
    return
  directory, rest = path.split(TEST_SEPARATOR, 1)
  if directory not in trie:
    trie[directory] = {}
  _AddPathToTrie(trie[directory], rest, value)


def RunTests(suite, args, verbosity):
  """Runs the tests in suite and reports results in Chromium JSON format."""
  result = unittest.TextTestRunner(verbosity=verbosity).run(suite)
  if args.write_full_results_to:
    with open(args.write_full_results_to, 'w') as fp:
      json.dump(_FullResults(suite, result, {}), fp, indent=2)
      fp.write('\n')
  return 0 if result.wasSuccessful() else 1


def ParseArgsAndRunTestInModules(modules):
  """Runs the tests in modules and reports results in Chromium JSON format."""
  parser = argparse.ArgumentParser()
  parser.add_argument('--write-full-results-to', metavar='FILENAME',
                      help='Path to write the list of full results to.')
  args = parser.parse_args()

  tests = []
  for module in modules:
    tests.append(unittest.defaultTestLoader.loadTestsFromModule(module))
  suite = unittest.TestSuite(tests)
  return RunTests(suite, args, verbosity=1)
