# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from telemetry.page import page as page_module
from telemetry import story

from page_sets import webgl_supported_shared_state


_MAP_PERF_TEST_DIR = os.path.join(os.path.dirname(__file__), 'map_perf_test')

class MapsPage(page_module.Page):
  """Google Maps benchmarks and pixel tests.

  The Maps team gave us a build of their test.
  """

  def __init__(self, page_set):
    super(MapsPage, self).__init__(
      url='file://performance.html',
      base_dir=_MAP_PERF_TEST_DIR,
      page_set=page_set,
      shared_page_state_class=(
          webgl_supported_shared_state.WebGLSupportedSharedState),
      name='map_perf_test')

  @property
  def skipped_gpus(self):
    # Skip this intensive test on low-end devices. crbug.com/464731
    return ['arm']

  def RunPageInteractions(self, action_runner):
    action_runner.WaitForJavaScriptCondition('window.startTest !== undefined')
    action_runner.EvaluateJavaScript('startTest()')
    with action_runner.CreateInteraction('MapAnimation'):
      action_runner.WaitForJavaScriptCondition(
        'window.testMetrics != undefined', timeout=120)


class MapsPageSet(story.StorySet):

  """ Google Maps examples """

  def __init__(self):
    super(MapsPageSet, self).__init__()

    self.AddStory(MapsPage(self))
