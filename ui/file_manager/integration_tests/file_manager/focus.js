// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

function getTouchData(x, y) {
  return {
    clientX: x, clientY: y,
  }
}

function getTouchEventData() {
  return {
    cancelable: true, bubbles: true,
  }
}

var headerQuery = ['.table-header'];
var x;
var y;

function longTappingTableHeaderSteps(appId) {
  return [
    function(results) {
      // Fetch the coordinates for tapping.
      repeatUntil(function() {
        return remoteCall
            .callRemoteTestUtil(
                'deepQueryAllElements', appId, [headerQuery, null])
            .then(function(results) {
              if (results.length === 0) {
                return pending('FilesApp table is not opened yet.');
              }
              return results;
            });
      }).then(this.next);
    },
    function(results) {
      chrome.test.assertTrue(results.length > 0)
      // Save the coordinates for tapping.
      x = results[0].renderedX;
      y = results[0].renderedY;
      this.next();
    },
    function() {
      // Put focus on the table by selecting a file with arrow keys.
      remoteCall.callRemoteTestUtil(
          'selectFile', appId, ['My Desktop Background.png'], this.next);
    },
    function(results) {
      chrome.test.assertTrue(results);
      // Start long tap.
      remoteCall.callRemoteTestUtil(
          'fakeTouchEvent', appId,
          [headerQuery, 'touchstart', getTouchEventData(), getTouchData(x, y)],
          this.next);
    },
    function(results) {
      chrome.test.assertTrue(results);
      // Wrap up long tap.
      var start = new Date().getTime();
      for (var i = 0; i < 1e7; i++) {
        if ((new Date().getTime() - start) > 3000) {
          break;
        }
      }
      remoteCall.callRemoteTestUtil(
          'fakeTouchEvent', appId,
          [headerQuery, 'touchend', getTouchEventData(), getTouchData(x, y)],
          this.next);
    },
    function(results) {
      chrome.test.assertTrue(results);
      // Check which element is in focus.
      remoteCall.callRemoteTestUtil(
          'getActiveElement', appId, [
            /* targetQuery */ null,
            /* iframeQuery */ null,
            /* opt_styleNames */ null,
          ],
          this.next);
    },
    function(element) {
      if (!element || !element.attributes.id ||
          element.attributes.id !== 'file-list') {
        chrome.test.fail('Focus on the detail file table lost.');
        console.log.error('IT IS NOT TRUE');
      }
      checkIfNoErrorsOccured(this.next);
    },
  ];
}

/**
 * Resize column by touch.
 */
testcase.longTappingDetailTableHeaderKeepsFocus = function() {
  setupAndWaitUntilReady(null, RootPath.DOWNLOADS).then(function(results) {
    StepsRunner.run(longTappingTableHeaderSteps(results.windowId));
  });
};
