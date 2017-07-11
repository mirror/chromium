# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from benchmarks import memory
from telemetry import benchmark
from vr_page_sets import webvr_sample_pure_gpu


@benchmark.Owner(emails=['bsheedy@chromium.org', 'leilei@chromium.org'])
class WebVrMemorySamplePureGpu(memory._MemoryInfra):
  """Measures WebVR memory on the official samples with only GPU options."""
  #TODO(bsheedy): repeat here?

  def CreateStorySet(self, options):
    return webvr_sample_pure_gpu.WebVrSamplePureGpuPageSet()

  def SetExtraBrowserOptions(self, options):
    super(WebVrMemorySamplePureGpu, self).SetExtraBrowserOptions(options)
    options.AppendExtraBrowserArgs([
      '--enable-webvr',
    ])

  @classmethod
  def Name(cls):
    return 'vr_memory.webvr_sample_pure_gpu'

  @classmethod
  def ShouldTearDownStateAfterEachStoryRun(cls):
    return True

  @classmethod
  def ValueCanBeAddedPredicate(cls, value, is_first_result):
    return not memory._IGNORED_STATS_RE.search(value.name)
