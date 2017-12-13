#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import sys
import unittest
import make_dafsa

import dafsa_utils


class ExamplesTest(unittest.TestCase):
  def testExample1(self):
    """Tests Example 1 from make_dafsa.py."""
    infile = '["https://www.example.com:8081", "http://www.example.org"]'
    outfile = "\n\x1c\x81www.example\xae\x02\x89com:8081\x80org\x81"
    self.assertEqual(make_dafsa.to_proto(dafsa_utils.words_to_bytes(
        make_dafsa.parse_json(infile))), outfile)

  def testExample2(self):
    """Tests Example 2 from make_dafsa.py."""
    infile = '["https://www.example.org", "http://www.google.com"]'
    outfile = "\n\x1e\x81www\xae\x02\x8bgoogle.com\x81example.org\x80"
    self.assertEqual(make_dafsa.to_proto(dafsa_utils.words_to_bytes(
        make_dafsa.parse_json(infile))), outfile)

  def testBadJSON(self):
    """Tests make_dafsa.py with bad JSON input."""
    self.assertRaises(dafsa_utils.InputError, make_dafsa.parse_json, "badinput")

  def testInvalidProtocol(self):
    """Tests make_dafsa.py with an invalid protocol."""
    self.assertRaises(dafsa_utils.InputError, make_dafsa.parse_json,
                      '["ftp://www.example.com"]')


if __name__ == '__main__':
  unittest.main()
