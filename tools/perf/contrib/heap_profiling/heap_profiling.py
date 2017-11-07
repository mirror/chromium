# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from benchmarks import loading_metrics_category

from core import perf_benchmark

from telemetry import story
from telemetry.timeline import chrome_trace_category_filter
from telemetry.timeline import chrome_trace_config
from telemetry.web_perf import timeline_based_measurement

import page_sets
from page_sets.system_health import loading_stories

_PAGE_SETS_DATA = os.path.join(os.path.dirname(page_sets.__file__), 'data')


class _HeapProfilingStorySet(story.StorySet):
  """Small story set containing loading stories and invoking memory dumps."""
  def __init__(self, platform):
    super(_HeapProfilingStorySet, self).__init__(
        archive_data_file=
            os.path.join(_PAGE_SETS_DATA, 'system_health_%s.json' % platform),
        cloud_storage_bucket=story.PARTNER_BUCKET)
    self.AddStory(
        loading_stories.LoadGoogleStory(self, take_memory_measurement=True))
    self.AddStory(
        loading_stories.LoadTwitterStory(self, take_memory_measurement=True))
    self.AddStory(
        loading_stories.LoadCnnStory(self, take_memory_measurement=True))


class _HeapProfilingBenchmark(perf_benchmark.PerfBenchmark):
  """Bechmark to measure heap profiling overhead."""
  BROWSER_OPTIONS = NotImplemented
  PLATFORM = NotImplemented

  def __init__(self, *args, **kwargs):
    super(_HeapProfilingBenchmark, self).__init__(*args, **kwargs)
    if self.PLATFORM == 'desktop':
      self.SUPPORTED_PLATFORMS = [story.expectations.ALL_DESKTOP]
    elif self.PLATFORM == 'mobile':
      self.SUPPORTED_PLATFORMS = [story.expectations.ALL_MOBILE]

  def CreateCoreTimelineBasedMeasurementOptions(self):
    cat_filter = chrome_trace_category_filter.ChromeTraceCategoryFilter(
        filter_string='-*,toplevel,rail,disabled-by-default-memory-infra')
    options = timeline_based_measurement.Options(cat_filter)
    options.SetTimelineBasedMetrics([
        'cpuTimeMetric',
        'loadingMetric',
        'memoryMetric',
        'tracingMetric',
    ])
    loading_metrics_category.AugmentOptionsForLoadingMetrics(options)
    # Disable periodic dumps by setting default config.
    options.config.chrome_trace_config.SetMemoryDumpConfig(
        chrome_trace_config.MemoryDumpConfig())
    return options

  def CreateStorySet(self, options):
    return _HeapProfilingStorySet(self.PLATFORM)

  def SetExtraBrowserOptions(self, options):
    super(_HeapProfilingBenchmark, self).SetExtraBrowserOptions(options)
    options.AppendExtraBrowserArgs(self.BROWSER_OPTIONS)

  def GetExpectations(self):
    class DefaultExpectations(story.expectations.StoryExpectations):
      def SetExpectations(self):
        pass  # No stories disabled.
    return DefaultExpectations()


class PseudoHeapProfilingDesktopBenchmark(_HeapProfilingBenchmark):
  BROWSER_OPTIONS = ['--enable-heap-profiling']
  PLATFORM = 'desktop'

  @classmethod
  def Name(cls):
    return 'heap_profiling.desktop.pseudo'


class NativeHeapProfilingDesktopBenchmark(_HeapProfilingBenchmark):
  BROWSER_OPTIONS = ['--enable-heap-profiling=native']
  PLATFORM = 'desktop'

  @classmethod
  def Name(cls):
    return 'heap_profiling.desktop.native'


class DisabledHeapProfilingDesktopBenchmark(_HeapProfilingBenchmark):
  BROWSER_OPTIONS = []
  PLATFORM = 'desktop'

  @classmethod
  def Name(cls):
    return 'heap_profiling.desktop.disabled'


class PseudoHeapProfilingMobileBenchmark(_HeapProfilingBenchmark):
  BROWSER_OPTIONS = ['--enable-heap-profiling']
  PLATFORM = 'mobile'

  @classmethod
  def Name(cls):
    return 'heap_profiling.mobile.pseudo'


class NativeHeapProfilingMobileBenchmark(_HeapProfilingBenchmark):
  BROWSER_OPTIONS = ['--enable-heap-profiling=native']
  PLATFORM = 'mobile'

  @classmethod
  def Name(cls):
    return 'heap_profiling.mobile.native'


class DisabledHeapProfilingMobileBenchmark(_HeapProfilingBenchmark):
  BROWSER_OPTIONS = []
  PLATFORM = 'mobile'

  @classmethod
  def Name(cls):
    return 'heap_profiling.mobile.disabled'
