# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from page_sets.system_health import system_health_story

class InspectorWindowManager(object):
  """...

  """
  def __init__(self, inspector_websocket):
    self._inspector_websocket = inspector_websocket
    self._inspector_websocket.RegisterDomain('WindowManager',
                                             self._OnNotification)
    self._EnableWindowManagerNotification()


  def EnterOverviewMode(self, timeout=60):
    request = { 'method': 'WindowManager.enterOverviewMode' }
    self._SendRequest(request, timeout)


  def ExitOverviewMode(self, timeout=60):
    request = { 'method': 'WindowManager.exitOverviewMode' }
    self._SendRequest(request, timeout)


  def _OnNotification(self, msg):
    logging.warning('Unhandled message: %s' % (msg))


  def _EnableWindowManagerNotification(self, timeout=60):
    request = { 'method': 'WindowManager.enable' }
    self._SendRequest(request, timeout)


  def _SendRequest(self, request, timeout=60):
    res = self._inspector_websocket.SyncRequest(request, timeout)
    assert len(res['result'].keys()) == 0


class WindowManagerOverview(system_health_story.SystemHealthStory):
  """Story that opens a new tab.

  Loads a website and enters a search query on omnibox and navigates to default
  search provider (google).
  """
  NAME = 'native_ui:browser:new_tab'
  URL = 'chrome:newtab'

  def __init__(self, story_set, take_memory_measurement):
    super(WindowManagerOverview, self).__init__(story_set,
        take_memory_measurement)
    self._wm = None

  def _OpenNewTabAndWaitForPaint(self, browser):
    tab = browser._browser_backend.tab_list_backend.New(10000)
    tab.Navigate('chrome:newtab')
    # Request a RAF and wait for it to be processed to ensure that the metric
    # Startup.FirstWebContents.NonEmptyPaint2 is recorded.
    tab.action_runner.ExecuteJavaScript(
        """
        window.__hasRunRAF = false;
        requestAnimationFrame(function() {
          window.__hasRunRAF = true;
        });
        """
    )
    tab.action_runner.WaitForJavaScriptCondition("window.__hasRunRAF")
    return tab

  def _DidLoadDocument(self, action_runner):
    browser = action_runner.tab.browser
    second = self._OpenNewTabAndWaitForPaint(browser)
    self._OpenNewTabAndWaitForPaint(browser)
    second.Activate()

    devtools_client = browser._browser_backend.devtools_client
    websocket = devtools_client._browser_inspector_websocket

    self._wm = InspectorWindowManager(websocket)
    self._wm.EnterOverviewMode()
    time.sleep(3)
    self._wm.ExitOverviewMode()
