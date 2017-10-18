# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.page import page as page_module
from telemetry import story


class _PageWithMemoryDump(page_module.Page):
  """Page without any interaction but capturing one memory dump."""
  def __init__(self, url, page_set):
    super(_PageWithMemoryDump, self).__init__(url, page_set=page_set, name=url)

  def RunPageInteractions(self, action_runner):
    action_runner.MeasureMemory(deterministic_mode=True)


class _GooglePage(_PageWithMemoryDump):
  def __init__(self, page_set):
    super(_GooglePage, self).__init__(
      url='https://www.google.com/#hl=en&q=barack+obama', page_set=page_set)

  def RunNavigateSteps(self, action_runner):
    super(_GooglePage, self).RunNavigateSteps(action_runner)
    action_runner.WaitForElement(text='Next')


class HeapProfilingPageSet(story.StorySet):
  def __init__(self):
    super(HeapProfilingPageSet, self).__init__(
      archive_data_file='data/top_10.json',
      cloud_storage_bucket=story.PARTNER_BUCKET)

    self.AddStory(_GooglePage(self))
