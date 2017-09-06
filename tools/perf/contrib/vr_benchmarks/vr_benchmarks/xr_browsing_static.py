# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry import benchmark
from telemetry.timeline import chrome_trace_category_filter
from telemetry.web_perf import timeline_based_measurement

from contrib.vr_benchmarks.vr_benchmarks.base_vr_benchmark import _BaseVRBenchmark
from contrib.vr_benchmarks.vr_page_sets import vr_browsing_mode_pages


@benchmark.Owner(emails=['tiborg@chromium.org'])
class XrBrowsingStatic(_BaseVRBenchmark):
  """Benchmark for testing the VR performance in VR Browsing Mode."""

  def CreateTimelineBasedMeasurementOptions(self):
    custom_categories = ['gpu']
    category_filter = chrome_trace_category_filter.ChromeTraceCategoryFilter(
        ','.join(custom_categories))
    options = timeline_based_measurement.Options(category_filter)
    options.config.enable_platform_display_trace = True
    options.SetTimelineBasedMetrics(['frameCycleDurationMetric'])
    return options

  def CreateStorySet(self, options):
    return vr_browsing_mode_pages.VrBrowsingModePageSet()

  def SetExtraBrowserOptions(self, options):
    options.clear_sytem_cache_for_browser_and_profile_on_start = True
    options.AppendExtraBrowserArgs([
        '--enable-gpu-benchmarking',
        '--touch-events=enabled',
        '--enable-vr-shell',
    ])

  @classmethod
  def Name(cls):
    return 'xr.browsing.static'
