// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.assertTrue(chrome.webrtcLoggingPrivate != null);
chrome.webrtcLoggingPrivate.getLogsDirectory(function (directoryEntry) {
  chrome.test.assertTrue(directoryEntry != null);
  chrome.test.assertEq(directoryEntry.name, 'WebRTC Logs');
  chrome.test.succeed();
});
