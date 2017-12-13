#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import dafsa_utils
import make_dafsa
import sys
import unittest


class ParseGperfTest(unittest.TestCase):
  def testMalformedKey(self):
    """Tests exception is thrown at bad format."""
    infile1 = [ '%%', '', '%%' ]
    self.assertRaises(dafsa_utils.InputError, make_dafsa.parse_gperf, infile1)

    infile2 = [ '%%', 'apa,1', '%%' ]
    self.assertRaises(dafsa_utils.InputError, make_dafsa.parse_gperf, infile2)

    infile3 = [ '%%', 'apa,  1', '%%' ]
    self.assertRaises(dafsa_utils.InputError, make_dafsa.parse_gperf, infile3)

  def testBadValues(self):
    """Tests exception is thrown when value is out of range."""
    infile1 = [ '%%', 'a, -1', '%%' ]
    self.assertRaises(dafsa_utils.InputError, make_dafsa.parse_gperf, infile1)

    infile2 = [ '%%', 'a, x', '%%' ]
    self.assertRaises(dafsa_utils.InputError, make_dafsa.parse_gperf, infile2)

    infile5 = [ '%%', 'a, 12', '%%' ]
    self.assertRaises(dafsa_utils.InputError, make_dafsa.parse_gperf, infile5)

  def testValues(self):
    """Tests legal values are accepted."""
    infile1 = [ '%%', 'a, 0', '%%' ]
    words1 = [ 'a0' ]
    self.assertEqual(make_dafsa.parse_gperf(infile1), words1)

    infile2 = [ '%%', 'a, 1', '%%' ]
    words2 = [ 'a1' ]
    self.assertEqual(make_dafsa.parse_gperf(infile2), words2)

    infile3 = [ '%%', 'a, 2', '%%' ]
    words3 = [ 'a2' ]
    self.assertEqual(make_dafsa.parse_gperf(infile3), words3)

    infile4 = [ '%%', 'a, 3', '%%' ]
    words4 = [ 'a3' ]
    self.assertEqual(make_dafsa.parse_gperf(infile4), words4)

    infile5 = [ '%%', 'a, 4', '%%' ]
    words5 = [ 'a4' ]
    self.assertEqual(make_dafsa.parse_gperf(infile5), words5)

    infile6 = [ '%%', 'a, 6', '%%' ]
    words6 = [ 'a6' ]
    self.assertEqual(make_dafsa.parse_gperf(infile6), words6)

  def testOneWord(self):
    """Tests a single key can be parsed."""
    infile = [ '%%', 'apa, 1', '%%' ]
    words = [ 'apa1' ]
    self.assertEqual(make_dafsa.parse_gperf(infile), words)

  def testTwoWords(self):
    """Tests a sequence of keys can be parsed."""
    infile = [ '%%', 'apa, 1', 'bepa.com, 2', '%%' ]
    words = [ 'apa1', 'bepa.com2' ]
    self.assertEqual(make_dafsa.parse_gperf(infile), words)


class ExamplesTest(unittest.TestCase):
  def testExample1(self):
    """Tests Example 1 from make_dafsa.py."""
    infile = [ '%%', 'aa, 1', 'a, 2', '%%' ]
    bytes = [ 0x81, 0xE1, 0x02, 0x81, 0x82, 0x61, 0x81 ]
    outfile = make_dafsa.to_cxx(bytes)
    self.assertEqual(make_dafsa.to_cxx(dafsa_utils.words_to_bytes(
        make_dafsa.parse_gperf(infile))), outfile)

  def testExample2(self):
    """Tests Example 2 from make_dafsa.py."""
    infile = [ '%%', 'aa, 1', 'bbb, 2', 'baa, 1', '%%' ]
    bytes = [ 0x02, 0x83, 0xE2, 0x02, 0x83, 0x61, 0x61, 0x81, 0x62, 0x62,
              0x82 ]
    outfile = make_dafsa.to_cxx(bytes)
    self.assertEqual(make_dafsa.to_cxx(dafsa_utils.words_to_bytes(
        make_dafsa.parse_gperf(infile))), outfile)


if __name__ == '__main__':
  unittest.main()
