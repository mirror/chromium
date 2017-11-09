// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var tests = [
  /**
   * Test that changes to window height bubble down to dropdowns correctly.
   */
  function testToolbarManagerResizeDropdown() {
    var mockWindow = new MockWindow(1920, 1080);
    var mockZoomToolbar = {
      clientHeight: 400
    };
    var toolbar = document.getElementById('toolbar');
    var bookmarksDropdown = toolbar.$.bookmarks;

    var toolbarManager =
        new ToolbarManager(mockWindow, toolbar, mockZoomToolbar);

    chrome.test.assertEq(680, bookmarksDropdown.lowerBound);

    mockWindow.setSize(1920, 480);
    chrome.test.assertEq(80, bookmarksDropdown.lowerBound);

    chrome.test.succeed();
  },
];

chrome.test.runTests(tests);
