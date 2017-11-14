// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function assertNoLastError(message) {
  if (chrome.runtime.lastError) {
    message += ': ' + chrome.runtime.lastError.message;
  }
  chrome.test.assertFalse(!!chrome.runtime.lastError, message);
}

function createWindowOnLoadHandler(win) {
  return function () {
    var webview =
        win.document.querySelector('webview');
    chrome.test.assertTrue(!!webview, 'webview not found');
    webview.addEventListener('loadstop', createWebviewAudioDebugTest(webview));
  };
}

function createWebviewAudioDebugTest(webview) {
  return function() {
    chrome.webrtcLoggingPrivate.startAudioDebugRecordings(
        {webview: webview}, '', 0,
        function(result) {
          assertNoLastError('startAudioDebugRecordings');
          chrome.webrtcLoggingPrivate.stopAudioDebugRecordings(
              {webview: webview}, '', function(result) {
                assertNoLastError('stopAudioDebugRecordings');
                chrome.test.succeed();
              });
        });
  };
}

chrome.app.runtime.onLaunched.addListener(function() {
      chrome.app.window.create(
          'test.html', {}, function(appWindow) {
            assertNoLastError('window.create');
            appWindow.contentWindow.onload =
                createWindowOnLoadHandler(appWindow.contentWindow);
          });
});
