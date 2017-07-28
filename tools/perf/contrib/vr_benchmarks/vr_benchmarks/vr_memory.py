# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from benchmarks import memory
from telemetry import benchmark
try:
  from vr_page_sets import webvr_sample_pages
except ImportError:
  # Make Pylint happy - doesn't think vr_page_sets can be imported
  from contrib.vr_benchmarks.vr_page_sets import webvr_sample_pages


@benchmark.Owner(emails=['bsheedy@chromium.org', 'leilei@chromium.org'])
class WebVrMemorySamplePages(memory._MemoryInfra):
  """Measures WebVR memory on an official sample page with settings tweaked."""

  def CreateStorySet(self, options):
    return webvr_sample_pages.WebVrSamplePageSet()

  def SetExtraBrowserOptions(self, options):
    super(WebVrMemorySamplePages, self).SetExtraBrowserOptions(options)
    options.AppendExtraBrowserArgs([
      '--enable-webvr',
    ])

  @classmethod
  def Name(cls):
    return 'vr_memory.webvr_sample_pages'

  @classmethod
  def ShouldTearDownStateAfterEachStoryRun(cls):
    return True

  @classmethod
  def ValueCanBeAddedPredicate(cls, value, is_first_result):
    return not memory._IGNORED_STATS_RE.search(value.name)
