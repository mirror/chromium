// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var pass = chrome.test.callbackPass;

chrome.tabs.getSelected(null, function(tab) {
  chrome.test.runTests([
    function getPopup() {
      chrome.action.getPopup({tabId: tab.id}, pass(function(result) {
        chrome.test.assertTrue(
            /chrome-extension\:\/\/[a-p]{32}\/Popup\.html/.test(result));
      }));
    },

    function getTitle() {
      chrome.action.getTitle({tabId: tab.id}, pass(function(result) {
        chrome.test.assertEq("Title", result);
      }));
    }
  ]);
});
