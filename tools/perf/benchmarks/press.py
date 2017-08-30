# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import os

from core import perf_benchmark
from telemetry import benchmark
from telemetry import decorators
from telemetry import page as page_module
from telemetry.page import legacy_page_test
from telemetry import story


class _PressMeasurement(legacy_page_test.LegacyPageTest):

  def __init__(self, benchmark_to_measure):
    super(_PressMeasurement, self).__init__()
    self.benchmark = benchmark_to_measure

  def ValidateAndMeasurePage(self, page, tab, results):
    self.benchmark.StartTest(page, tab, results)
    self.benchmark.WaitForTestToFinish(page, tab, results)
    self.benchmark.ParseTestResults(page, tab, results)

@decorators.Disabled('all')
@benchmark.Owner(emails=['ashleymarie@chromium.org'])
class PressBenchmark(perf_benchmark.PerfBenchmark):
  """A base class for Press benchmarks."""
  base_dir = os.path.dirname(os.path.abspath(__file__))
  serving_dirs = [base_dir]
  stories = []
  name = None

  @classmethod
  def Name(cls):
    return cls.name

  def CreateStorySet(self, options):
    """Makes a PageSet for Dromaeo benchmarks."""
    ps = story.StorySet(
        archive_data_file='../page_sets/data/press.json',
        base_dir=self.base_dir,
        cloud_storage_bucket=story.INTERNAL_BUCKET,
        serving_dirs=self.serving_dirs)
    for name, url in self.stories:
      ps.AddStory(page_module.Page(
          url, ps, ps.base_dir, make_javascript_deterministic=False, name=name))
    return ps

  def CreatePageTest(self, options):
    return _PressMeasurement(self)

  def StartTest(self, page, tab, results):
    raise NotImplementedError()

  def WaitForTestToFinish(self, page, tab, results):
    raise NotImplementedError()

  def ParseTestResults(self, page, tab, results):
    raise NotImplementedError()

  def GetExpectations(self):
    class StoryExpectations(story.expectations.StoryExpectations):
      def SetExpectations(self):
        pass # press benchmark not disabled.
    return StoryExpectations()
