#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import sys
import unittest
import make_dafsa


class ExamplesTest(unittest.TestCase):
  def testExample1(self):
    """Tests Example 1 from make_dafsa.py."""
    infile = '["https://www.example.com:8081", "http://www.example.org"]'
    outfile = ("\n3\x81htt\xf0\x02\x93://www.example.or\xe7\x99"
               "s://www.example.com:8081\x80")
    self.assertEqual(make_dafsa.words_to_proto(make_dafsa.parse_json(infile)),
                      outfile)

  def testExample2(self):
    """Tests Example 2 from make_dafsa.py."""
    infile = '["https://www.example.org", "http://www.google.com"]'
    outfile = ("\n-\x81htt\xf0\x02\x92://www.google.co\xed\x94"
               "s://www.example.org\x80")
    self.assertEqual(make_dafsa.words_to_proto(make_dafsa.parse_json(infile)),
                      outfile)

  def testExample3(self):
    """Tests Example 3 from make_dafsa.py."""
    self.assertRaises(
        make_dafsa.dafsa_utils.InputError, make_dafsa.parse_json, "badinput")


if __name__ == '__main__':
  unittest.main()
