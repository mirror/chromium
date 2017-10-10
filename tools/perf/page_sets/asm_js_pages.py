# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry import story


class EsenthelPage(page_module.Page):
  def __init__(self, page_set):
    super(EsenthelPage, self).__init__(
        url='http://www.esenthel.com/?id=live_demo',
        page_set=page_set)
    self.archive_data_file='data/asmjs_page_set.json'

  def RunNavigateSteps(self, action_runner):
    super(EsenthelPage, self).RunNavigateSteps(action_runner)
    action_runner.Wait(30)


class AsmJsPageSet(story.StorySet):

  """ Page set consists of pages using asm.js. """

  def __init__(self):
    super(AsmJsPageSet, self).__init__(
        archive_data_file='data/asmjs_page_set.json',
        cloud_storage_bucket=story.PARTNER_BUCKET)
    self.AddStory(EsenthelPage(self))
