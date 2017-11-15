# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry import story


class EpicZenGarden(page_module.Page):

  def __init__(self, page_set):
    url = 'https://s3.amazonaws.com/mozilla-games/ZenGarden/EpicZenGarden.html'
    super(EpicZenGarden, self).__init__(
        url=url, page_set=page_set, name=url)

  def RunPageInteractions(self, action_runner):
    # We wait for 20 seconds to make sure we capture enough information
    # to calculate the interactive time correctly.
    action_runner.WaitForJavaScriptCondition(
        """document.getElementById('fullscreen_request').style.display ==
          'inline-block'""")


class WasmRealWorldPerfStorySet(story.StorySet):
  """Top apps, used to monitor web assembly apps."""

  def __init__(self):
    super(WasmRealWorldPerfStorySet, self).__init__(
        archive_data_file='data/wasm_realworld_perf.json',
        cloud_storage_bucket=story.INTERNAL_BUCKET)

    self.AddStory(EpicZenGarden(self))
