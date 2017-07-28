# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


from telemetry import page
from telemetry import story
from telemetry.core import exceptions
from telemetry.page import shared_page_state
from telemetry.util import js_template


class SharedState(shared_page_state.SharedPageState):
  """Shared state that restarts the browser for every single story."""

  def __init__(self, test, finder_options, story_set):
    super(SharedState, self).__init__(
        test, finder_options, story_set)

  def DidRunStory(self, results):
    super(SharedState, self).DidRunStory(results)
    self._StopBrowser()


class WebVRPage(page.Page):
  """WebVR page."""

  def __init__(self, page_set, url='https://webvr.info/samples/test-slow-render.html?canvasClickPresents=1&renderScale=1',
               shared_page_state_class=shared_page_state.SharedPageState,
               name='vr_presentation.html'):
    super(WebVRPage, self).__init__(
        url=url, page_set=page_set,
        shared_page_state_class=shared_page_state_class, name=name)

  def RunPageInteractions(self, action_runner):
    # Wait for 5s after Chrome is opened in order to get consistent results.
    action_runner.Wait(5)
    with action_runner.CreateInteraction('WebVR'):
      action_runner.TapElement(selector='#webgl-canvas')
      action_runner.Wait(10)


class WebVRPageSet(story.StorySet):
  """Pageset for webvr tests."""

  def __init__(self):
    super(WebVRPageSet, self).__init__(
        cloud_storage_bucket=story.PARTNER_BUCKET)
    self.AddStory(WebVRPage(self))
