// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var pass = chrome.test.callbackPass;

chrome.test.runTests([
  function getBadgeText() {
    chrome.action.getBadgeText({}, pass(function(result) {
      chrome.test.assertEq("Text", result);
    }));
  },

  function getBadgeBackgroundColor() {
    chrome.action.getBadgeBackgroundColor({}, pass(function(result) {
      chrome.test.assertEq([255, 0, 0, 255], result);
    }));
  },

  function getPopup() {
    chrome.action.getPopup({}, pass(function(result) {
      chrome.test.assertTrue(
          /chrome-extension\:\/\/[a-p]{32}\/Popup\.html/.test(result));
    }));
  },

  function getTitle() {
    chrome.action.getTitle({}, pass(function(result) {
      chrome.test.assertEq("Title", result);
    }));
  }
]);
