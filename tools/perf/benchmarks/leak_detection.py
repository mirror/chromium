# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from core import perf_benchmark
from telemetry import benchmark
from telemetry.timeline import chrome_trace_category_filter
from telemetry.timeline import chrome_trace_config
from telemetry.web_perf import timeline_based_measurement

from page_sets import leak_detection

@benchmark.Owner(emails=['yuzus@chromium.org'])
class MemoryLeakDetectionBenchmark(perf_benchmark.PerfBenchmark):
  page_set = leak_detection.LeakDetectionStorySet

  @classmethod
  def Name(cls):
    return 'memory.leak_detection'

  def CustomizeBrowserOptions(self, cls, options):
    options.AppendExtraBrowserArgs('--enable-logging')
    options.AppendExtraBrowserArgs('--v=1')
    options.AppendExtraBrowserArgs('--no-sandbox')
    options.AppendExtraBrowserArgs('--js-flags=--expose-gc')

  def CreateCoreTimelineBasedMeasurementOptions(self):
    tbm_options = timeline_based_measurement.Options(
        chrome_trace_category_filter.ChromeTraceCategoryFilter(
            '-*,disabled-by-default-memory-infra'))
    # Setting an empty memory dump config disables periodic dumps.
    tbm_options.config.chrome_trace_config.SetMemoryDumpConfig(
        chrome_trace_config.MemoryDumpConfig())
    # Set required tracing categories for leak detection
    tbm_options.AddTimelineBasedMetric('leakDetectionMetric')
    return tbm_options
