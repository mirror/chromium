// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

self.onmessage = function(e) {
  var data = e.data;
  chrome.test.assertEq('testServiceWorkerFilteredEvents', data);
  runTest();
};

function runTest() {
  var getURL = chrome.extension.getURL;
  chrome.tabs.create({'url': 'about:blank'}, function(tab) {
    var tabId = tab.id;
    var aVisited = false;
    chrome.webNavigation.onCommitted.addListener(function(details) {
      chrome.test.fail();
    }, { url: [{pathSuffix: 'never-navigated.html'}] });
    chrome.webNavigation.onCommitted.addListener(function(details) {
      chrome.test.log('chrome.webNavigation.onCommitted - a.html');
      chrome.test.assertTrue(details.url == getURL('a.html'));
      aVisited = true;
    }, { url: [{pathSuffix: 'a.html'}] });
    chrome.webNavigation.onCommitted.addListener(function(details) {
      chrome.test.log('chrome.webNavigation.onCommitted - b.html');
      chrome.test.assertTrue(details.url == getURL('b.html'));
      chrome.test.assertTrue(aVisited);
      chrome.test.succeed();
    }, { url: [{pathSuffix: 'b.html'}] });

    chrome.tabs.update(tabId, { url: getURL('a.html') });
  });
}
