# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry import story


class WebVrSamplePureGpuPage(page_module.Page):
  def __init__(self, get_parameters, page_set, wait_time=5):
    # TODO(bsheedy): change to actual url
    #url = 'test-slow-render.html'
    url = 'test_telemetry_page.html'
    self._wait_time = wait_time
    if get_parameters:
      url += '?' + '&'.join(get_parameters)
    name = '%s_%d_seconds' % (url, wait_time)
    # TODO(bsheedy): change to actual url
    url = 'file://../../../../../chrome/test/data/android/webvr_instrumentation/html/' + url
    super(WebVrSamplePureGpuPage, self).__init__(url=url, page_set=page_set,
                                                 name=name)

  def RunPageInteractions(self, action_runner):
      action_runner.TapElement(selector='canvas[id="webgl-canvas"]')
      action_runner.Wait(self._wait_time)
      action_runner.MeasureMemory(True)


class WebVrSamplePureGpuPageSet(story.StorySet):
  """A collection of WebVR sample pages that only touch GPU options."""

  def __init__(self):
    super(WebVrSamplePureGpuPageSet, self).__init__()
    self.AddStory(WebVrSamplePureGpuPage([], self, wait_time=5))
    self.AddStory(WebVrSamplePureGpuPage([], self, wait_time=15))
