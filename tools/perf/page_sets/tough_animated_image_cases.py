# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story

class ToughAnimatedImageCasesPage(page_module.Page):

  def __init__(self, url, name, page_set):
    super(ToughAnimatedImageCasesPage, self).__init__(
      url=url,
      page_set=page_set,
      name=name,
      shared_page_state_class=shared_page_state.SharedMobilePageState)
    self.archive_data_file = 'data/tough_animated_image_cases.json'

  def RunPageInteractions(self, action_runner):
    action_runner.WaitForJavaScriptCondition(
      'document.readyState === "complete"')
    with action_runner.CreateInteraction('ImageAnimation'):
      action_runner.Wait(5)

class ToughAnimatedImageCasesPageSet(story.StorySet):

  """
  Description: A collection of difficult animated image cases.
  """

  def __init__(self):
    super(ToughAnimatedImageCasesPageSet, self).__init__(
      archive_data_file='data/tough_animated_image_cases.json',
      cloud_storage_bucket=story.PUBLIC_BUCKET)

    page_name_list = [
      'https://giphy.com/search/60-fps',
    ]

    for name in page_name_list:
      self.AddStory(ToughAnimatedImageCasesPage(
        name, name, self))
