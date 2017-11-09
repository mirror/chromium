// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var tests = [
  function testGetDirectory() {
    chrome.test.assertTrue(chrome.webrtcLoggingPrivate != null);
    chrome.webrtcLoggingPrivate.getLogsDirectory(function(directoryEntry) {
      chrome.test.assertTrue(directoryEntry != null);
      chrome.test.assertEq('WebRTC Logs', directoryEntry.name);
      chrome.test.succeed();
    });
  },
];

chrome.test.runTests(tests);
