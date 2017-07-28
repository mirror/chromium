# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from telemetry import benchmark
from core import perf_benchmark
from core import path_util
from telemetry.timeline import chrome_trace_category_filter
from telemetry.web_perf import timeline_based_measurement

import webvr_pagesets
import fps_metric


class _BaseVRBenchmark(perf_benchmark.PerfBenchmark):
  options = {'pageset_repeat': 1}

  page_set = webvr_pagesets.WebVRPageSet

  def SetExtraBrowserOptions(self, options):
    options.clear_sytem_cache_for_browser_and_profile_on_start = True
    options.AppendExtraBrowserArgs('--enable-gpu-benchmarking')
    options.AppendExtraBrowserArgs('--touch-events=enabled')
    options.AppendExtraBrowserArgs(['--enable-webvr', '--no-restore-state', '--disable-fre'])


class WebVRBenckmark(_BaseVRBenchmark):
  """Benchmark for WebVR."""

  def CreateTimelineBasedMeasurementOptions(self):
    custom_categories = [
        'webkit.console', 'blink.console', 'benchmark', 'gpu']
    category_filter = chrome_trace_category_filter.ChromeTraceCategoryFilter(
        ','.join(custom_categories))
    options = timeline_based_measurement.Options(category_filter)
    options.config.enable_platform_display_trace = True
    options.SetLegacyTimelineBasedMetrics([
        fps_metric.WebVRFpsMetric()])
    return options

  @classmethod
  def Name(cls):
    return 'webvr'

  @classmethod
  def ValueCanBeAddedPredicate(cls, value, is_first_result):
    """Only drops the first result."""
    return True
    #return not is_first_result
