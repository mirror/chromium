// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


// Runs the |tests| using the |tab| as a default tab.
function runTestsForTab(tests, tab) {
  tabId = tab.id;
  chrome.test.getConfig(function(config) {
    testServerPort = config.testServer.port;
    chrome.test.runTests(tests);
  });
}

// Creates an "about:blank" tab and runs |tests| with this tab as default.
function runTests(tests) {
  var waitForAboutBlank = function(_, info, tab) {
    if (info.status == "complete" && tab.url == "about:blank") {
      chrome.tabs.onUpdated.removeListener(waitForAboutBlank);
      runTestsForTab(tests, tab);
    }
  };
  chrome.tabs.onUpdated.addListener(waitForAboutBlank);
  chrome.tabs.create({url: "about:blank"});
}

// Helper to advance to the next test only when the tab has finished loading.
// This is because tabs.update can sometimes fail if the tab is in the middle
// of a navigation (from the previous test), resulting in flakiness.
function navigateAndWait(url, callback) {
  var done = chrome.test.listenForever(chrome.tabs.onUpdated,
      function (_, info, tab) {
    if (tab.id == tabId && info.status == "complete") {
      done();
      if (callback) callback();
    }
  });
  chrome.tabs.update(tabId, {url: url});
}

// Returns an URL from the test server, fixing up the port. Must be called
// from within a test case passed to runTests.
function getServerURL(path, host) {
  if (!testServerPort)
    throw new Error("Called getServerURL outside of runTests.");
  if (!host)
    throw new Error("Called getServerURL without a host.");
  return "http://" + host + ":" + testServerPort + "/" + path;
}


