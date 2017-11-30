# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.page import page as page_module
from telemetry import story

from page_sets import webgl_supported_shared_state


class MapsPage(page_module.Page):
  """Google Maps benchmarks and pixel tests.

  The Maps team gave us a build of their performance test
  (chrome_smoothness_performancetest). The files were served up on
  localhost on port 8000.

  Below, "http://map-test" was replaced with "http://localhost:8000". The
  WPR was then recorded with:

  tools/perf/record_wpr smoothness_maps --browser=system

  This produced maps_???.wprgo, maps_???.wprgo.sha1 and maps.json.

  Telemetry no longer allows replaying a URL that refers to localhost. The
  archive then needs to be updated via:

  TODO(kbr): need https://github.com/catapult-project/catapult/issues/3787
  fixed and a command line to invoke web_page_replay_go's httparchive to
  remap localhost:8000 to map-test in the recorded archive

  Then update the maps.json file to delete all references to the old
  archives. There should be only one reference to a maps_???.wprgo archive
  in that file.

  After updating the host name in the WPR archive, or recording a
  newly-numbered WPR archive, please remember to update
  content/test/gpu/gpu_tests/maps_integration_test.py (and potentially
  its pixel expectations) as well.

  To upload the maps_???.wprgo to cloud storage, one would run:

    depot_tools/upload_to_google_storage.py --bucket=chromium-telemetry \
    maps_???.wprgo

  """

  def __init__(self, page_set):
    url = 'http://map-test/performance.html'
    super(MapsPage, self).__init__(
      url=url,
      page_set=page_set,
      shared_page_state_class=(
          webgl_supported_shared_state.WebGLSupportedSharedState),
      name=url)

  @property
  def skipped_gpus(self):
    # Skip this intensive test on low-end devices. crbug.com/464731
    return ['arm']

  def RunNavigateSteps(self, action_runner):
    super(MapsPage, self).RunNavigateSteps(action_runner)
    action_runner.WaitForJavaScriptCondition('window.startTest != undefined')

  def RunPageInteractions(self, action_runner):
    # Kick off the map's animation.
    action_runner.EvaluateJavaScript('window.startTest()')

    with action_runner.CreateInteraction('MapAnimation'):
      action_runner.WaitForJavaScriptCondition(
        'window.testMetrics != undefined', timeout=120)


class MapsPageSet(story.StorySet):

  """ Google Maps examples """

  def __init__(self):
    super(MapsPageSet, self).__init__(
        archive_data_file='data/maps.json',
        cloud_storage_bucket=story.PUBLIC_BUCKET)

    self.AddStory(MapsPage(self))
