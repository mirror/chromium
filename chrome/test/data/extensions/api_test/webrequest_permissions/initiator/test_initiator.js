// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Tab where the content script has been injected.
var testTabId;

// Port where the test server is accessible.
var testServerPort;

function getTestUrl(host) {
  return 'http://' + host + ':' + testServerPort +
      '/extensions/test_file.txt';
}

function getTestInitiator(host) {
  if (host)
    return 'http://' + host + ':' + testServerPort;
  return undefined;
}

// Navigate to provided URL and wait for content script to be injected then
// signal that test can proceed.
function navigateAndWait(url, callback) {
  chrome.runtime.onMessage.addListener(
      function listener(request, sender, sendResponse) {
        chrome.test.assertEq('injected', request);
        chrome.runtime.onMessage.removeListener(listener);
        callback();
      });
  chrome.tabs.update(testTabId, {url: url});
}

// Add listeners for webRequest events and signal extension to send XHR to URL.
// Filter webRequest events & assert that initiator & destination match expected
// values.
function xhrToHost(host, expectedInitiator) {
  var expectedDestination = getTestUrl(host);
  chrome.webRequest.onBeforeRequest.addListener(function listener(details) {
    if ((details.url != expectedDestination) ||
        (details.type != 'xmlhttprequest')) {
      return;
    }
    chrome.webRequest.onBeforeRequest.removeListener(listener);
    chrome.test.assertEq(expectedInitiator, details.initiator);
    chrome.test.succeed();
  }, {urls: ['*://*/*']}, []);

  chrome.tabs.sendMessage(testTabId, {'url': expectedDestination});
}

// Helper for each initiator test.
function testInitiator(hostInitiating, destinationHost, expectedInitiator) {
  navigateAndWait(getTestUrl(hostInitiating), function() {
    xhrToHost(destinationHost, getTestInitiator(expectedInitiator));
  });
}

chrome.test.getConfig(function(config) {
  testServerPort = config.testServer.port;
  chrome.tabs.create({url: 'about:blank'}, function(tab) {
    testTabId = tab.id;
    chrome.test.runTests([
      function sameSite() {
        testInitiator('example.com', 'example.com', 'example.com');
      },
      function crossDomainVisible() {
        testInitiator('example2.com', 'example3.com', 'example2.com');
      },
      function crossDomainInvisible() {
        testInitiator('no-permission.com', 'example4.com', undefined);
      }
    ]);
  });
});


