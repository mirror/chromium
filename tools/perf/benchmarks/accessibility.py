# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from core import perf_benchmark

import page_sets

from telemetry import benchmark
from telemetry import story
from telemetry.timeline import chrome_trace_category_filter
from telemetry.web_perf import timeline_based_measurement


@benchmark.Owner(emails=['dmazzoni@chromium.org'])
class AccessibilityBenchmark(perf_benchmark.PerfBenchmark):
  """Measures time spent in accessibility code"""

  @classmethod
  def Name(cls):
    return 'accessibility'

  @classmethod
  def ShouldDisable(cls, possible_browser):
    return False

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--force-renderer-accessibility')

  def GetExpectations(self):
    class StoryExpectations(story.expectations.StoryExpectations):
      def SetExpectations(self):
        pass # Nothing disabled.
    return StoryExpectations()

  def CreateCoreTimelineBasedMeasurementOptions(self):
    cat_filter = chrome_trace_category_filter.ChromeTraceCategoryFilter()

    cat_filter.AddIncludedCategory('accessibility')

    tbm_options = timeline_based_measurement.Options(
        overhead_level=cat_filter)
    tbm_options.SetTimelineBasedMetrics(['accessibilityMetric'])
    return tbm_options

  def CreateStorySet(self, options):
    return page_sets.AccessibilityPageSet()
