// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// signin - not logged in, screen not locked.
chrome.test.runTests([
  function getLoginStatus() {
    chrome.authPrivate.getLoginStatus(
        chrome.test.callbackPass(function(status) {
          chrome.test.assertEq(typeof(status), 'object');
          chrome.test.assertTrue(status.hasOwnProperty("isLoggedIn"));
          chrome.test.assertTrue(status.hasOwnProperty("isScreenLocked"));
          //chrome.test.assertFalse(status.isLoggedIn);
          chrome.test.assertFalse(status.isScreenLocked);
        }));
  },
]);
