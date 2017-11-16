// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function assertNoLastError(message) {
  if (chrome.runtime.lastError) {
    message += ': ' + chrome.runtime.lastError.message;
  }
  chrome.test.assertFalse(!!chrome.runtime.lastError, message);
}

function executeWindowTest(testBody) {
    chrome.app.window.create(
        'appwindow.html', {}, function(appWindow) {
          assertNoLastError('window.create');
          appWindow.contentWindow.onload = function() {
            testBody(appWindow.contentWindow);
          };
        });
}

function createWebviews(doc, n, callback) {
  var loaded = 0;
  var webview;
  function onLoadStop() {
    loaded++;
    if (loaded == n) {
      callback();
    }
  }
  for (var i = 0; i < n; i++) {
    webview = doc.createElement('webview');
    webview.src = 'data:text/plain, test';
    webview.addEventListener('loadstop', onLoadStop);
    doc.body.appendChild(webview);
  }
}

function testStartStopWithOneWebview() {
  executeWindowTest(function(win) {
    createWebviews(win.document, 1, function() {
      win.attemptAudioDebugRecording(
          function() {
            chrome.test.succeed('Started and stopped with 1 webview');
          }, chrome.test.fail);
    });
  });
}

function testFailWithMoreThanOneWebview() {
  executeWindowTest(function(win) {
    createWebviews(win.document, 2, function() {
      win.attemptAudioDebugRecording(
          function() {
            chrome.test.fail('Expected runtime error');
          },
          chrome.test.succeed);
    });
  });
}

function testFailWithZeroWebviews() {
  executeWindowTest(function(win) {
    win.attemptAudioDebugRecording(
        function() {
          chrome.test.fail('Expected runtime error');
        },
        chrome.test.succeed);
  });
}

chrome.app.runtime.onLaunched.addListener(function() {
  chrome.test.runTests([
    testStartStopWithOneWebview,
    testFailWithMoreThanOneWebview,
    testFailWithZeroWebviews,
  ]);
});
