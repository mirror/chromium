# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from benchmarks import leak_detection as ld

from contrib.cluster_telemetry import ct_benchmarks_util
from contrib.cluster_telemetry import page_set

from telemetry import story


# pylint: disable=protected-access
class LeakDetectionClusterTelemetry(ld.MemoryLeakDetectionBenchmark):

  options = {'upload_results': True}

  @classmethod
  def AddBenchmarkCommandLineArgs(cls, parser):
    super(LeakDetectionClusterTelemetry, cls).AddBenchmarkCommandLineArgs(parser)
    ct_benchmarks_util.AddBenchmarkCommandLineArgs(parser)

  def CreateStorySet(self, options):
    def RunNavigateSteps(action_runner):
        action_runner.Navigate('about:blank')
        action_runner.PrepareForLeakDetection()
        action_runner.MeasureMemory(True)
        action_runner.Navigate(self.url)
        py_utils.WaitFor(action_runner.tab.HasReachedQuiescence, timeout=30)
        action_runner.Navigate('about:blank')
        action_runner.PrepareForLeakDetection()
        action_runner.MeasureMemory(True)
    return page_set.CTPageSet(
      options.urls_list, options.user_agent, options.archive_data_file,
      run_page_interaction_callback=RunNavigateSteps)

  @classmethod
  def Name(cls):
    return 'leak_detection.cluster_telemetry'

  def GetExpectations(self):
    class StoryExpectations(story.expectations.StoryExpectations):
      def SetExpectations(self):
        pass # Not tests disabled.
    return StoryExpectations()
