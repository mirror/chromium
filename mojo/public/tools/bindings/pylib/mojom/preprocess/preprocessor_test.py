# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Run this test from the pylib directory with the command:
# python -m mojom.preprocess.preprocessor_test
import unittest

from mojom.preprocess import preprocessor


class PreprocessorTest(unittest.TestCase):

  def Process(self, expression, buildflags=None, defines=()):
    return preprocessor._ProcessConditional('t.mojom', 1, expression, buildflags
                                            if buildflags else {}, defines)

  def testBuildflags(self):
    self.assertTrue(
        self.Process('BUILDFLAG(PLUGINS)', buildflags={'PLUGINS': 1}))
    self.assertFalse(
        self.Process('BUILDFLAG(PLUGINS)', buildflags={'PLUGINS': 0}))
    self.assertFalse(
        self.Process('BUILDFLAG(PLUGINS)', buildflags={'EXTENSIONS': 1}))

  def testDefines(self):
    self.assertTrue(self.Process('defined(OS_WIN)', defines=('OS_WIN')))
    self.assertFalse(self.Process('defined(OS_WIN)', defines=('OS_MACOSX')))

  def testNegation(self):
    self.assertFalse(
        self.Process('!BUILDFLAG(PLUGINS)', buildflags={'PLUGINS': 1}))
    self.assertTrue(
        self.Process('!BUILDFLAG(PLUGINS)', buildflags={'EXTENSIONS': 1}))
    self.assertFalse(self.Process('!defined(OS_WIN)', defines=('OS_WIN')))
    self.assertTrue(self.Process('!defined(OS_WIN)', defines=('OS_MACOSX')))

  def testAnd(self):
    self.assertFalse(
        self.Process(
            'BUILDFLAG(PLUGINS) && defined(OS_WIN)', buildflags={'PLUGINS': 1}))
    self.assertTrue(
        self.Process(
            'BUILDFLAG(PLUGINS) && !defined(OS_WIN)', buildflags={'PLUGINS': 1
                                                                 }))
    self.assertFalse(
        self.Process(
            '!BUILDFLAG(PLUGINS) && !defined(OS_WIN)',
            buildflags={'PLUGINS': 1}))
    self.assertFalse(
        self.Process(
            '!BUILDFLAG(PLUGINS) && defined(OS_WIN)', buildflags={'PLUGINS': 1
                                                                 }))

  def testOr(self):
    self.assertTrue(
        self.Process(
            'BUILDFLAG(PLUGINS) || defined(OS_WIN)', buildflags={'PLUGINS': 1}))
    self.assertTrue(
        self.Process(
            'BUILDFLAG(PLUGINS) || !defined(OS_WIN)', buildflags={'PLUGINS': 1
                                                                 }))
    self.assertTrue(
        self.Process(
            '!BUILDFLAG(PLUGINS) || !defined(OS_WIN)',
            buildflags={'PLUGINS': 1}))
    self.assertFalse(
        self.Process(
            '!BUILDFLAG(PLUGINS) || defined(OS_WIN)', buildflags={'PLUGINS': 1
                                                                 }))

  # Note: negation is implicitly tested by the 'and' and 'or' test cases above.
  def testAndOrPrecedence(self):
    self.assertTrue(
        self.Process(
            'defined(OS_WIN) || BUILDFLAG(PLUGINS) && BUILDFLAG(EXTENSIONS)',
            defines=('OS_WIN')))


if __name__ == '__main__':
  unittest.main()
