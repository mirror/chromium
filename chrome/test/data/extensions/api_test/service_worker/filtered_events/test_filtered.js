// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/*
onload = function() {
  var getURL = chrome.extension.getURL;
  chrome.tabs.create({"url": "about:blank"}, function(tab) {
    var tabId = tab.id;
    chrome.test.runTests([
      function dontGetEventToWrongUrl() {
        var a_visited = false;
        chrome.webNavigation.onCommitted.addListener(function(details) {
          chrome.test.fail();
        }, { url: [{pathSuffix: 'never-navigated.html'}] });
        chrome.webNavigation.onCommitted.addListener(function(details) {
          chrome.test.assertTrue(details.url == getURL('a.html'));
          a_visited = true;
        }, { url: [{pathSuffix: 'a.html'}] });
        chrome.webNavigation.onCommitted.addListener(function(details) {
          chrome.test.assertTrue(details.url == getURL('b.html'));
          chrome.test.assertTrue(a_visited);
          chrome.test.succeed();
        }, { url: [{pathSuffix: 'b.html'}] });
        chrome.tabs.update(tabId, { url: getURL('a.html') });
      }
    ]);
  });
};
*/

var workerPromise = new Promise(function(resolve, reject) {
  navigator.serviceWorker.register('sw.js').then(function() {
    return navigator.serviceWorker.ready;
  }).then(function(registration) {
    var sw = registration.active;
    var channel = new MessageChannel();
    channel.port1.onmessage = function(e) {
      var data = e.data;
      if (data == 'listener-added') {
        var url = chrome.extension.getURL('on_updated.html');
        chrome.tabs.create({url: url});
      } else if (data == 'chrome.tabs.onUpdated callback') {
        resolve(data);
      } else if (data == 'TEST_COMPLETE') {
        resolve(data);
      } else {
        reject(data);
      }
    };
    sw.postMessage('testFilteredEvents', [channel.port2]);
  }).catch(function(err) {
    reject(err);
  });
});


function testServiceWorkerFilteredEvents() {
  workerPromise.then(function(message) {
    console.log('**** workerPromise: message = ' + message);
    //chrome.test.succeed();
    if (message == 'TEST_COMPLETE') {
      chrome.test.succeed();
    }
  }).catch(function(err) {
    console.log('**** workerPromise: err = ' + err);
    chrome.test.fail();
  });
};

// Start worker with sw.js, postMessage startTest.
chrome.test.runTests([
  testServiceWorkerFilteredEvents
]);

//window.runEventTest = function() {
//  workerPromise.then(function(message) {
//    window.domAutomationController.send(message);
//  }).catch(function(err) {
//    window.domAutomationController.send('FAILURE');
//  });
//};
