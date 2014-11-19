# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import shutil
import tempfile

from measurements import screenshot
from telemetry import benchmark
from telemetry.unittest_util import options_for_unittests
from telemetry.unittest_util import page_test_test_case


class ScreenshotUnitTest(page_test_test_case.PageTestTestCase):
  def setUp(self):
    self._options = options_for_unittests.GetCopy()
    self._png_outdir = tempfile.mkdtemp('_png_test')

  def tearDown(self):
    shutil.rmtree(self._png_outdir)

  @benchmark.Disabled('win')  # http://crbug.com/386572
  def testScreenshot(self):
    ps = self.CreatePageSetFromFileInUnittestDataDir('blank.html')
    measurement = screenshot.Screenshot(self._png_outdir)
    results = self.RunMeasurement(measurement, ps, options=self._options)
    if results.failures:
      logging.warning(str(results.failures))
      return

    saved_picture_count = results.FindAllPageSpecificValuesNamed(
        'saved_picture_count')
    self.assertEquals(len(saved_picture_count), 1)
    self.assertGreater(saved_picture_count[0].GetRepresentativeNumber(), 0)
