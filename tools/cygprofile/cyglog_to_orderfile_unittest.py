#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import os
import sys
import unittest

import cyglog_to_orderfile
import process_profiles
import symbol_extractor

from test_utils import (SymbolInfo,
                        TestSymbolOffsetProcessor)


sys.path.insert(
    0, os.path.join(os.path.dirname(__file__), os.pardir, os.pardir,
                    'third_party', 'android_platform', 'development',
                    'scripts'))
import symbol

SymbolWithSection = collections.namedtuple(
    'SymbolWithSection',
    ['name', 'offset', 'size', 'section'])


class TestGenerator(cyglog_to_orderfile.OffsetOrderfileGenerator):
  def __init__(self, symbol_infos, symbol_to_sections=None):
    # Replace superclass initialization with our own.
    self._offset_to_symbols = TestSymbolOffsetProcessor(
        symbol_infos).OffsetToSymbolsMap()
    self._symbol_to_sections = symbol_to_sections


class TestCyglogToOrderfile(unittest.TestCase):
  def testParseLogLines(self):
    lines = """5086e000-52e92000 r-xp 00000000 b3:02 51276      libchromeview.so
secs       usecs      pid:threadid    func
START
1314897086 795828     3587:1074648168 0x509e105c
1314897086 795874     3587:1074648168 0x509e0eb4
END""".split('\n')
    offsets = cyglog_to_orderfile._ParseLogLines(lines)
    self.assertListEqual(
        offsets, [0x509e105c - 0x5086e000, 0x509e0eb4 - 0x5086e000])

  def testWarnAboutDuplicates(self):
    offsets = [0x1, 0x2, 0x3]
    self.assertTrue(cyglog_to_orderfile._WarnAboutDuplicates(offsets))
    offsets.append(0x1)
    self.assertFalse(cyglog_to_orderfile._WarnAboutDuplicates(offsets))

  def testSymbolsAtOffsetExactMatch(self):
    symbol_infos = [SymbolInfo('1', 0x10, 0x13)]
    generator = TestGenerator(symbol_infos)
    syms = generator._SymbolsAtOffset(0x10)
    self.assertEquals(1, len(syms))
    self.assertEquals(symbol_infos[0], syms[0])

  def testSymbolsAtOffsetInexectMatch(self):
    symbol_infos = [SymbolInfo('1', 0x10, 0x13)]
    generator = TestGenerator(symbol_infos)
    syms = generator._SymbolsAtOffset(0x11)
    self.assertEquals(1, len(syms))
    self.assertEquals(symbol_infos[0], syms[0])

  def testSameCtorOrDtorNames(self):
    if not os.path.exists(symbol.ToolPath('c++filt')):
      print 'Skipping test dependent on missing c++filt binary.'
      return
    same_name = (cyglog_to_orderfile.OffsetOrderfileGenerator
                 ._SameCtorOrDtorNames)
    self.assertTrue(same_name(
        '_ZNSt3__119istreambuf_iteratorIcNS_11char_traitsIcEEEC1Ev',
        '_ZNSt3__119istreambuf_iteratorIcNS_11char_traitsIcEEEC2Ev'))
    self.assertTrue(same_name(
        '_ZNSt3__119istreambuf_iteratorIcNS_11char_traitsIcEEED1Ev',
        '_ZNSt3__119istreambuf_iteratorIcNS_11char_traitsIcEEED2Ev'))
    self.assertFalse(same_name(
        '_ZNSt3__119istreambuf_iteratorIcNS_11char_traitsIcEEEC1Ev',
        '_ZNSt3__119foo_iteratorIcNS_11char_traitsIcEEEC1Ev'))
    self.assertFalse(same_name(
        '_ZNSt3__119istreambuf_iteratorIcNS_11char_traitsIcEEE',
        '_ZNSt3__119istreambuf_iteratorIcNS_11char_traitsIcEEE'))

  def testOutputOrderfile(self):
    # One symbol not matched, one with an odd address, one regularly matched
    # And two symbols aliased to the same address
    symbols = [SymbolWithSection('Symbol', 0x10, 0x100, 'dummy'),
               SymbolWithSection('Symbol2', 0x12, 0x100, 'dummy'),
               SymbolWithSection('Symbol3', 0x16, 0x100, 'dummy'),
               SymbolWithSection('Symbol3.2', 0x16, 0x0, 'dummy')]
    generator = TestGenerator(symbols, {
        'Symbol': ['.text.Symbol'],
        'Symbol2': ['.text.Symbol2', '.text.hot.Symbol2'],
        'Symbol3': ['.text.Symbol3'],
        'Symbol3.2': ['.text.Symbol3.2']})
    ordered_sections = generator.GetOrderedSections([0x12, 0x17])
    self.assertListEqual(
        ['.text.Symbol2',
         '.text.hot.Symbol2',
         '.text.Symbol3',
         '.text.Symbol3.2'],
        ordered_sections)


if __name__ == '__main__':
  unittest.main()
