# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from benchmarks import memory
from telemetry import benchmark
from contrib.vr_benchmarks.vr_page_sets import webvr_sample_pages
from contrib.vr_benchmarks.vr_benchmarks.base_vr_benchmark import _BaseVRBenchmark


@benchmark.Owner(emails=['bsheedy@chromium.org', 'leilei@chromium.org'])
class WebVrMemorySamplePages(_BaseVRBenchmark):
  """Measures WebVR memory on an official sample page with settings tweaked."""

  def CreateCoreTimelineBasedMeasurementOptions(self):
    return memory.CreateCoreTimelineBasedMemoryMeasurementOptions()

  def CreateStorySet(self, options):
    return webvr_sample_pages.WebVrSamplePageSet()

  def SetExtraBrowserOptions(self, options):
    memory.SetExtraBrowserOptionsForMemoryMeasurement(options)
    options.AppendExtraBrowserArgs([
        '--enable-webvr',
    ])

  @classmethod
  def Name(cls):
    return 'vr_memory.webvr_sample_pages'

  @classmethod
  def ValueCanBeAddedPredicate(cls, value, is_first_result):
    return memory.DefaultValueCanBeAddedPredicateForMemoryMeasurement(value)
