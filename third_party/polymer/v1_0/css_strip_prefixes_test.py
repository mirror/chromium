#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import css_strip_prefixes
import unittest


class CssStripPrefixesTest(unittest.TestCase):
  def testShouldRemoveLineTrue(self):
    # Test case where the prefixed property is before the the colon.
    self.assertTrue(css_strip_prefixes.ShouldRemoveLine('-ms-flex: bar;'))
    self.assertTrue(css_strip_prefixes.ShouldRemoveLine('-ms-flex:bar;'))
    self.assertTrue(css_strip_prefixes.ShouldRemoveLine('  -ms-flex: bar; '))
    self.assertTrue(css_strip_prefixes.ShouldRemoveLine('  -ms-flex: bar ;'))

    # Test case where the prefixed property is after the the colon.
    self.assertTrue(
        css_strip_prefixes.ShouldRemoveLine(' display: -ms-inline-flexbox;'))

    # Test lines with comments also get removed.
    self.assertTrue(css_strip_prefixes.ShouldRemoveLine(
        ' display: -ms-inline-flexbox; /* */'))
    self.assertTrue(css_strip_prefixes.ShouldRemoveLine(
        ' -ms-flex: bar; /* foo */ '))

  def testShouldRemoveLineFalse(self):
    self.assertFalse(css_strip_prefixes.ShouldRemoveLine(''))
    self.assertFalse(css_strip_prefixes.ShouldRemoveLine(' -ms-flex'))
    self.assertFalse(css_strip_prefixes.ShouldRemoveLine('/* -ms-flex */'))
    self.assertFalse(css_strip_prefixes.ShouldRemoveLine(': -ms-flex; '))
    self.assertFalse(css_strip_prefixes.ShouldRemoveLine(' : -ms-flex; '))
    self.assertFalse(css_strip_prefixes.ShouldRemoveLine('-ms-flex {')):


if __name__ == '__main__':
  unittest.main()
