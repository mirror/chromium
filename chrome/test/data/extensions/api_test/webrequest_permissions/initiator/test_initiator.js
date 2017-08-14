// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


// Tab where the content script has been injected.
var testTabId;

// Port where the test server is accessible.
var testServerPort;

var pass = chrome.test.callbackPass;

function getTestUrl(host) {
  return 'http://' + host + ':' + testServerPort +
      '/extensions/test_file.txt';
}

function getTestInitiator(host) {
  if (host)
    return 'http://' + host + ':' + testServerPort;
  else
    return undefined;
}

// Navigate to provided URL and wait for content script to be injected then
// signal that test can proceed.
function navigateAndWait(url, callback) {
  chrome.runtime.onMessage.addListener(
      function(request, sender, sendResponse) {
        chrome.test.assertEq('injected', request);
        chrome.runtime.onMessage.removeListener(arguments.callee);
        callback();
      });
  chrome.tabs.update(testTabId, {url: url});
}

// Filter webRequest events & assert that initiator & destination match expected
// values.
function beforeRequestListener(details, expected_destination,
    expected_initiator) {
      if ((details.url != expected_destination) ||
          (details.type != 'xmlhttprequest')) {
        return;
      }
      chrome.webRequest.onBeforeRequest.removeListener(arguments.callee);
      chrome.test.assertEq(expected_initiator, details.initiator);
      chrome.test.succeed();
}

// Add listeners for webRequest events and signal extension to send XHR to URL.
function xhrToHost(host, expected_initiator) {
  var expected_destination = getTestUrl(host);
  chrome.webRequest.onBeforeRequest.addListener(function(details) {
    beforeRequestListener(details, expected_destination, expected_initiator);
  }, {urls: ['*://*/*']}, []);

  chrome.tabs.sendMessage(testTabId, {'url': expected_destination});
}

// Helper for each initiator test.
function testInitiator(host_initiating, destination_host, expected_initiator) {
  navigateAndWait(getTestUrl(host_initiating), function() {
    xhrToHost(destination_host, getTestInitiator(expected_initiator));
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


