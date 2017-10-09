// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function getURL(port, filename) {
  return 'http://127.0.0.1:' + port +
      '/extensions/api_test/service_worker/filtered_events_after_restart/' +
      filename;
}

function pass() { chrome.test.sendMessage('PASS_FROM_WORKER'); }
function fail() { chrome.test.sendMessage('FAIL_FROM_WORKER'); }

var seenURLs = [];
var recordCommitAndVerify = function(committedURL) {
  seenURLs.push(committedURL);

  if (seenURLs.length > 2) {
    fail();
    return;
  }
  if (seenURLs.length != 2) {
    return;
  }

  chrome.test.getConfig(function(config) {
    var port = config.testServer.port;
    var passed =
        seenURLs[0] == getURL(port, 'a.html') &&
        seenURLs[1] == getURL(port, 'b.html');
    passed ? pass() : fail();
  });
};

var registerFilteredEventListeners = function() {
  // We expect a.html to commit first and then b.html to commit.
  chrome.webNavigation.onCommitted.addListener(function(details) {
    fail();
  }, {url: [{pathSuffix: 'never-navigated.html'}]});
  chrome.webNavigation.onCommitted.addListener(function(details) {
    chrome.test.log('webNavigation.onCommitted, a.html: ' + details.url);
    recordCommitAndVerify(details.url);
  }, {url: [{pathSuffix: 'a.html'}]});
  chrome.webNavigation.onCommitted.addListener(function(details) {
    chrome.test.log('webNavigation.onCommitted, b.html: ' + details.url);
    recordCommitAndVerify(details.url);
  }, {url: [{pathSuffix: 'b.html'}]});
};

registerFilteredEventListeners();
