# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import py_utils
from telemetry import story
from telemetry.page import page as page_module


class LeakDetectionPage(page_module.Page):
  def __init__(self, url, page_set, name=''):
    if name == '':
      name = url
    if len(name) >= 180:
      name = name[:179]

    super(LeakDetectionPage, self).__init__(
      url=url, page_set=page_set, name=name,
      credentials_path = 'data/credentials.json')

  def RunNavigateSteps(self, action_runner):
    action_runner.Navigate('about:blank')
    action_runner.PrepareForLeakDetection()
    action_runner.MeasureMemory(True)
    action_runner.Navigate(self.url)
    py_utils.WaitFor(action_runner.tab.HasReachedQuiescence, timeout=30)
    action_runner.Navigate('about:blank')
    action_runner.PrepareForLeakDetection()
    action_runner.MeasureMemory(True)

urls_list = [
    'https://www.google.com',
]

class LeakDetectionStorySet(story.StorySet):
  def __init__(self):
    super(LeakDetectionStorySet, self).__init__(
      cloud_storage_bucket=story.PARTNER_BUCKET)
    for url in urls_list:
      self.AddStory(LeakDetectionPage(url, self, url))
