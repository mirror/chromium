# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.page import legacy_page_test

from metrics import usecounter

class UseCounter(legacy_page_test.LegacyPageTest):

  def __init__(self, needs_browser_restart_after_each_page=False):
      super(UseCounter, self).__init__(needs_browser_restart_after_each_page)
      self._metric = usecounter.UseCounterMetric()

  @classmethod
  def CustomizeBrowserOptions(cls, options):
    usecounter.UseCounterMetric.CustomizeBrowserOptions(options)

  def WillNavigateToPage(self, page, tab):
    self._metric.Start(page, tab)

  def DidNavigateToPage(self, page, tab):
    self._metric.Stop(page, tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    self._metric.AddResults(tab, results)
