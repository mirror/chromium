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

function getMouseEventData(x, y) {
  return {
    cancelable: true, bubbles: true, clientX: x, clientY: y,
  }
}

var splitterQuery = ['.table-header-splitter'];
var oldX;
var oldY;
var moveX;

function resizeColumnSteps(appId, inputType) {
  return [
    function() {
      // Wait until FilesApp has finished loading.
      repeatUntil(function() {
        // Fetch a reference to the HTMLElement that represents the column.
        return remoteCall
            .callRemoteTestUtil(
                'deepQueryAllElements', appId, [splitterQuery, null])
            .then(function(results) {
              if (results.length === 0) {
                return pending('FilesApp table is not opened yet.');
              }
              return results;
            });
      }).then(this.next);
    },
    function(results) {
      chrome.test.assertTrue(results.length > 0);
      // Save the old position of the splitter.
      oldX = results[0].renderedX;
      oldY = results[0].renderedY;
      moveX = -10;
      this.next();
    },
    function() {
      // Start resizing the column.
      switch (inputType) {
        case 'touch':
          return remoteCall.callRemoteTestUtil(
              'fakeTouchEvent', appId,
              [
                splitterQuery, 'touchstart', getTouchEventData(),
                getTouchData(oldX, oldY)
              ],
              this.next);
        case 'mouse':
          return remoteCall.callRemoteTestUtil(
              'fakeEvent', appId,
              [splitterQuery, 'mousedown', getMouseEventData(oldX, oldY)],
              this.next);
      }
    },
    function(results) {
      chrome.test.assertTrue(results);
      // Move the position of the column.
      switch (inputType) {
        case 'touch':
          return remoteCall.callRemoteTestUtil(
              'fakeTouchEvent', appId,
              [
                splitterQuery, 'touchmove', getTouchEventData(),
                getTouchData(oldX + moveX, oldY)
              ],
              this.next);
        case 'mouse':
          return remoteCall.callRemoteTestUtil(
              'fakeEvent', appId,
              [
                splitterQuery, 'mousemove',
                getMouseEventData(oldX + moveX, oldY)
              ],
              this.next);
      }
    },
    function(results) {
      chrome.test.assertTrue(results);
      // Finish resizing the column.
      switch (inputType) {
        case 'touch':
          return remoteCall.callRemoteTestUtil(
              'fakeTouchEvent', appId,
              [
                splitterQuery, 'touchend', getTouchEventData(),
                getTouchData(oldX + moveX, oldY)
              ],
              this.next);
        case 'mouse':
          return remoteCall.callRemoteTestUtil(
              'fakeEvent', appId,
              [splitterQuery, 'mouseup', getMouseEventData(oldX + moveX, oldY)],
              this.next);
      }
    },
    function(results) {
      chrome.test.assertTrue(results);
      // Wait until FilesApp has finished loading.
      repeatUntil(function() {
        // Fetch a reference to the HTMLElement that represents the column.
        return remoteCall
            .callRemoteTestUtil(
                'deepQueryAllElements', appId, [splitterQuery, null])
            .then(function(results) {
              if (results.length === 0) {
                return pending('FilesApp table is not opened yet.');
              }
              return results;
            });
      }).then(this.next);
    },
    function(results) {
      chrome.test.assertTrue(results.length > 0);
      // Check the column was actually resized.
      chrome.test.assertEq(oldX + moveX, results[0].renderedX);
      chrome.test.assertEq(oldY, results[0].renderedY);
      checkIfNoErrorsOccured(this.next);
    },
  ];
}

/**
 * Resize column by touch.
 */
testcase.resizeColumnByTouch = function() {
  setupAndWaitUntilReady(null, RootPath.DOWNLOADS).then(function(results) {
    StepsRunner.run(resizeColumnSteps(results.windowId, 'touch'));
  });
};

/**
 * Resize column by mouse pointer.
 */
testcase.resizeColumnByMouse = function() {
  setupAndWaitUntilReady(null, RootPath.DOWNLOADS).then(function(results) {
    StepsRunner.run(resizeColumnSteps(results.windowId, 'mouse'));
  });
};
