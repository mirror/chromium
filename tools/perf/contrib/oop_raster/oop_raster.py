# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from core import perf_benchmark
from measurements import smoothness,thread_times
from benchmarks import silk_flags
import page_sets
from telemetry import benchmark


# copy from benchmarks/smoothness.py
class _Smoothness(perf_benchmark.PerfBenchmark):
  """Base class for smoothness-based benchmarks."""

  # Certain smoothness pages do not perform gesture scrolling, in turn yielding
  # an empty first_gesture_scroll_update_latency result. Such empty results
  # should be ignored, allowing aggregate metrics for that page set.
  _PAGES_WITHOUT_SCROLL_GESTURE_BLACKLIST = [
      'http://mobile-news.sandbox.google.com/news/pt0']

  test = smoothness.Smoothness

  @classmethod
  def Name(cls):
    return 'smoothness'

  @classmethod
  def ValueCanBeAddedPredicate(cls, value, is_first_result):
    del is_first_result  # unused
    if (value.name == 'first_gesture_scroll_update_latency' and
        value.page.url in cls._PAGES_WITHOUT_SCROLL_GESTURE_BLACKLIST and
        value.values is None):
      return False
    return True


# copy from benchmarks/thread_times.py
class _ThreadTimes(perf_benchmark.PerfBenchmark):

  @classmethod
  def AddBenchmarkCommandLineArgs(cls, parser):
    parser.add_option('--report-silk-details', action='store_true',
                      help='Report details relevant to silk.')

  @classmethod
  def Name(cls):
    return 'thread_times'

  @classmethod
  def ValueCanBeAddedPredicate(cls, value, _):
    # Default to only reporting per-frame metrics.
    return 'per_second' not in value.name

  def SetExtraBrowserOptions(self, options):
    silk_flags.CustomizeBrowserOptionsForThreadTimes(options)

  def CreatePageTest(self, options):
    return thread_times.ThreadTimes(options.report_silk_details)


def CustomizeBrowserOptionsForOopRasterization(options):
  """Enables flags needed for out of process rasterization."""
  options.AppendExtraBrowserArgs('--force-gpu-rasterization')
  options.AppendExtraBrowserArgs('--enable-oop-rasterization')


@benchmark.Owner(emails=['enne@chromium.org'])
class SmoothnessOopRasterizationTop25(_Smoothness):
  """Measures rendering statistics for the top 25 with oop rasterization.
  """
  tag = 'oop_rasterization'
  page_set = page_sets.Top25SmoothPageSet

  def SetExtraBrowserOptions(self, options):
    CustomizeBrowserOptionsForOopRasterization(options)

  @classmethod
  def Name(cls):
    return 'smoothness.oop_rasterization.top_25_smooth'


@benchmark.Owner(emails=['enne@chromium.org'])
class ThreadTimesOopRasterKeyMobile(_ThreadTimes):
  """Measure timeline metrics for key mobile pages while using out of process
  raster."""
  tag = 'oop_rasterization'
  page_set = page_sets.KeyMobileSitesSmoothPageSet

  def SetExtraBrowserOptions(self, options):
    super(ThreadTimesOopRasterKeyMobile, self).SetExtraBrowserOptions(options)
    CustomizeBrowserOptionsForOopRasterization(options)

  @classmethod
  def Name(cls):
    return 'thread_times.oop_rasterization.key_mobile'
