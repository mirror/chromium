// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var expectedEventData;
var capturedEventData;

function expect(data) {
  expectedEventData = data;
  capturedEventData = [];
}

var messagePort = null;
function sendMessage(msg) { messagePort.postMessage(msg); }

function checkExpectations() {
  if (capturedEventData.length < expectedEventData.length) {
    return;
  }

  var passed = JSON.stringify(expectedEventData) ==
      JSON.stringify(capturedEventData);
  if (passed) {
    sendMessage('chrome.tabs.onUpdated callback');
  } else {
    sendMessage('FAILURE');
  }
}

//self.onmessage = function(e) {
//  var data = e.data;
//  messagePort = e.ports[0];
//  if (data == 'addOnUpdatedListener') {
//    addOnUpdatedListener();
//    e.ports[0].postMessage('listener-added');
//  }
//};
self.onmessage = function(e) {
  var data = e.data;
  messagePort = e.ports[0];
  if (data == 'testFilteredEvents') {
    //testFilteredEvents();
    testWebNavigationEventBasic();
    //e.ports[0].postMessage('listener-added');
  }
};

function testWebNavigationEventBasic() {
  console.log(chrome.extension);
  var getURL = chrome.extension.getURL;
  //sendMessage('TEST_COMPLETE');
  chrome.tabs.create({'url': 'about:blank'}, function(tab) {
    var tabId = tab.id;
    console.log('*** created, tabId = ' + tabId + ' ***');
    chrome.webNavigation.onCommitted.addListener(function(details) {
      console.log('*** onCommitted ***');
      sendMessage('TEST_COMPLETE');
      chrome.test.succeed();
    });
    console.log('url = ' + getURL('a.html'));
    chrome.tabs.update(tabId, {url: getURL('a.html')}, function() {
      console.log('*** onUpdated ***');
    });
  });
}

//function testFilteredEvents() {
//  var getURL = chrome.extension.getURL;
//  chrome.tabs.create({"url": "about:blank"}, function(tab) {
//    var tabId = tab.id;
//    chrome.test.runTests([
//      function dontGetEventToWrongUrl() {
//        var a_visited = false;
//        chrome.webNavigation.onCommitted.addListener(function(details) {
//          chrome.test.fail();
//        }, { url: [{pathSuffix: 'never-navigated.html'}] });
//        chrome.webNavigation.onCommitted.addListener(function(details) {
//          chrome.test.assertTrue(details.url == getURL('a.html'));
//          a_visited = true;
//        }, { url: [{pathSuffix: 'a.html'}] });
//        chrome.webNavigation.onCommitted.addListener(function(details) {
//          chrome.test.assertTrue(details.url == getURL('b.html'));
//          chrome.test.assertTrue(a_visited);
//          chrome.test.succeed();
//        }, { url: [{pathSuffix: 'b.html'}] });
//        chrome.tabs.update(tabId, { url: getURL('a.html') });
//      }
//    ]);
//  });
//}

//function addOnUpdatedListener() {
//  chrome.tabs.onUpdated.addListener(function(tabId, info, tab) {
//    console.log('onUpdated, tabId: ' + tabId + ', info: ' +
//        JSON.stringify(info) + ', tab: ' + JSON.stringify(tab));
//    capturedEventData.push(info);
//    checkExpectations();
//  });
//
//  var url = chrome.extension.getURL('on_updated.html');
//  expect([
//    {status: 'loading', 'url': url},
//    {status: 'complete'},
//    {title: 'foo'},
//    {title: 'bar'}
//  ]);
//};
