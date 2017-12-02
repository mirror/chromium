#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for generate_buildbot_json.py."""

import unittest

import generate_buildbot_json


class FakeBBGen(generate_buildbot_json.BBJSONGenerator):
  def __init__(self, waterfalls, test_suites, exceptions):
    super(FakeBBGen, self).__init__()
    self.files = {
      'waterfalls.pyl': waterfalls,
      'test_suites.pyl': test_suites,
      'test_suite_exceptions.pyl': exceptions,
    }

  def read_file(self, relative_path):
    return self.files[relative_path]


UNREFED_TEST_SUITE_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'testers': {
      'Fake Tester': {
        'test_suites': {
          'gtest_tests': 'foo_gtests',
        },
      },
    },
  },
]
"""

GOOD_TEST_SUITES = """\
{
  'foo_gtests': {},
}
"""

UNREFED_TEST_SUITE = """\
{
  'foo_gtests': {},
  'bar_gtests': {},
}
"""

EMPTY_EXCEPTIONS = """\
{
}
"""

class UnitTest(unittest.TestCase):
  def test_good_test_suites_are_ok(self):
    fbb = FakeBBGen(UNREFED_TEST_SUITE_WATERFALL,
                    GOOD_TEST_SUITES,
                    EMPTY_EXCEPTIONS)
    fbb.check_input_file_consistency()

  def test_unrefed_test_suite_caught(self):
    fbb = FakeBBGen(UNREFED_TEST_SUITE_WATERFALL,
                    UNREFED_TEST_SUITE,
                    EMPTY_EXCEPTIONS)
    self.assertRaises(generate_buildbot_json.BBGenErr,
                      fbb.check_input_file_consistency)

if __name__ == '__main__':
  unittest.main()

