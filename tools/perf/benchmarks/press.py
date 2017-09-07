# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import os

from core import perf_benchmark
from py_utils import discover
from telemetry import benchmark
from telemetry import page as page_module
from telemetry.page import legacy_page_test
from telemetry import story


class _MetaPressPage(type):
  """Metaclass for PressPage."""

  @property
  def ABSTRACT_PAGE(cls):
    """Class field marking whether the class is abstract.

    If true, the page will NOT be instantiated and added to a Press
    story set. This field is NOT inherited by subclasses (that's why it's
    defined on the metaclass).
    """
    return cls.__dict__.get('ABSTRACT_PAGE', False)


class PressPage(page_module.Page):
  """Abstract base class for Press user pages."""
  __metaclass__ = _MetaPressPage

  ABSTRACT_PAGE = True
  NAME = None
  URL = None

  def __init__(self, pageset):
    super(PressPage, self).__init__(self.URL, pageset, pageset.base_dir,
                                    make_javascript_deterministic = False,
                                    name=self.NAME)

  def StartTest(self, page, tab, results):
    raise NotImplementedError()

  def WaitForTestToFinish(self, page, tab, results):
    raise NotImplementedError()

  def ParseTestResults(self, page, tab, results):
    raise NotImplementedError()


class _PressMeasurement(legacy_page_test.LegacyPageTest):

  def ValidateAndMeasurePage(self, page, tab, results):
    page.StartTest(page, tab, results)
    page.WaitForTestToFinish(page, tab, results)
    page.ParseTestResults(page, tab, results)


@benchmark.Owner(emails=['ashleymarie@chromium.org'])
class PressBenchmark(perf_benchmark.PerfBenchmark):
  test = _PressMeasurement

  @classmethod
  def Name(cls):
    return 'press_benchmark'

  def CreateStorySet(self, options):
    """Makes a PageSet for Dromaeo benchmarks."""
    base_dir = os.path.dirname(os.path.abspath(__file__))
    ps = story.StorySet(
        archive_data_file='../page_sets/data/press.json',
        base_dir=base_dir,
        cloud_storage_bucket=story.INTERNAL_BUCKET,
        serving_dirs=[base_dir])
    for _, story_class in sorted(
        discover.DiscoverClasses(
            start_dir=base_dir,
            top_level_dir=os.path.dirname(base_dir),
            base_class=PressPage).iteritems()):
          if not story_class.ABSTRACT_PAGE:
            ps.AddStory(story_class(ps))
    return ps


  def GetExpectations(self):
    class StoryExpectations(story.expectations.StoryExpectations):
      def SetExpectations(self):
        pass # press benchmark not disabled.
    return StoryExpectations()
