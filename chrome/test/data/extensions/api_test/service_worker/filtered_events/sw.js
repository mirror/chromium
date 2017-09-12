// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var messagePort = null;
function sendMessage(msg) { messagePort.postMessage(msg); }

self.onmessage = function(e) {
  var data = e.data;
  messagePort = e.ports[0];
  chrome.test.assertEq('testFilteredEvents', data);
  if (data == 'testFilteredEvents') {
    testFilteredEvents();
    //testWebNavigationEventBasic();
    //e.ports[0].postMessage('listener-added');
  }
};

//// THIS IS A PASS.
//function testWebNavigationEventBasic() {
//  console.log(chrome.extension);
//  var getURL = chrome.extension.getURL;
//  //sendMessage('TEST_COMPLETE');
//  chrome.tabs.create({'url': 'about:blank'}, function(tab) {
//    var tabId = tab.id;
//    console.log('*** created, tabId = ' + tabId + ' ***');
//    chrome.webNavigation.onCommitted.addListener(function(details) {
//      console.log('*** onCommitted ***');
//      //sendMessage('TEST_COMPLETE');
//      chrome.test.succeed();
//    });
//    console.log('url = ' + getURL('a.html'));
//    chrome.tabs.update(tabId, {url: getURL('a.html')}, function() {
//      console.log('*** onUpdated ***');
//    });
//  });
//}

function testFilteredEvents() {
  var getURL = chrome.extension.getURL;
  chrome.tabs.create({"url": "about:blank"}, function(tab) {
    var tabId = tab.id;
    console.log('begin');
    var aVisited = false;
    chrome.webNavigation.onCommitted.addListener(function(details) {
      chrome.test.fail();
    }, { url: [{pathSuffix: 'never-navigated.html'}] });
    chrome.webNavigation.onCommitted.addListener(function(details) {
      console.log('onCommitted 1');
      chrome.test.assertTrue(details.url == getURL('a.html'));
      aVisited = true;
    }, { url: [{pathSuffix: 'a.html'}] });
    chrome.webNavigation.onCommitted.addListener(function(details) {
      console.log('onCommitted 2');
      chrome.test.assertTrue(details.url == getURL('b.html'));
      chrome.test.assertTrue(aVisited);
      chrome.test.succeed();
    }, { url: [{pathSuffix: 'b.html'}] });
    chrome.tabs.update(tabId, { url: getURL('a.html') });
  });
}
