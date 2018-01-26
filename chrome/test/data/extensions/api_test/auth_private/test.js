// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.runTests([
  function getLoginStatus(function callback) {
    chrome.autotestPrivate.getLoginStatus(
        chrome.test.callbackPass(function(status) {
          chrome.test.assertEq(typeof(status), 'object');
          chrome.test.assertTrue(status.hasOwnProperty("isLoggedIn"));
          chrome.test.assertTrue(status.hasOwnProperty("isScreenLocked"));
        }));
  },
]);
