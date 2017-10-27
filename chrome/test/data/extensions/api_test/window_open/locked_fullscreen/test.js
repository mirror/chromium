// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.runTests([
  function openLockedFullscreenWindow() {
    chrome.windows.create({url: 'about:blank', state: 'locked'},
      chrome.test.callbackPass(function(window) {
        // Make sure the new window state is correctly set.
        chrome.test.assertEq(window.state, 'locked');
    }));
  }
]);
