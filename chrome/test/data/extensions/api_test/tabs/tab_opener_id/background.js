// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Add a listener just so we dispatch onUpdated events.
chrome.tabs.onUpdated.addListener(function(update) {});

// Test a series of crazy events where we can set a tab's opener, then close it,
// and hope the browser does the right thing.
// Regression test for crbug.com/698681.
chrome.test.runTests([
  function testSetOpenerOutsideOfWindow() {
    chrome.test.getConfig((config) => {
      var getUrl = function(url) {
        return 'http://localhost:' + config.testServer.port + '/' + url;
      };

      var url1 = getUrl('title1.html');
      var url2 = getUrl('title2.html');
      var url3 = getUrl('title3.html');

      // create a new window with two tabs (url1 and url2).
      chrome.windows.create({url: [url1, url2]}, (win) => {
        chrome.test.assertNoLastError();
        chrome.test.assertTrue(!!win);
        chrome.test.assertTrue(!!win.tabs);
        chrome.test.assertEq(2, win.tabs.length);
        var openerId = win.tabs[0].id;

        // Create a second window with a single tab (url3).
        chrome.windows.create({url: url3}, (secondWin) => {
          chrome.test.assertNoLastError();
          chrome.test.assertTrue(!!secondWin);
          chrome.test.assertTrue(!!secondWin.tabs);
          chrome.test.assertEq(1, secondWin.tabs.length);
          var tabIdSecondWin = secondWin.tabs[0].id;

          // Try to update the tab in the second window to have an opener of a
          // tab in the first window. This *should* fail.
          chrome.tabs.update(tabIdSecondWin, {openerTabId: openerId}, () => {
            chrome.test.assertLastError(
                'Tab opener must be in the same window as the updated tab.');

            // Next, remove the tab we tried to set as the opener.
            chrome.tabs.remove(openerId, () => {
              chrome.test.assertNoLastError();

              // And finally, query the tabs.
              chrome.tabs.query({}, function(tabs) {
                chrome.test.assertNoLastError();
                chrome.test.succeed();
              });
            });
          });
        });
      });
    });
  },

  function testSetOpenerToSelf() {
    chrome.test.getConfig((config) => {
      var getUrl = function(url) {
        return 'http://localhost:' + config.testServer.port + '/' + url;
      };

      var url1 = getUrl('title1.html');

      // create a new window with one tab.
      chrome.windows.create({url: [url1]}, (win) => {
        chrome.test.assertNoLastError();
        chrome.test.assertTrue(!!win);
        chrome.test.assertTrue(!!win.tabs);
        chrome.test.assertEq(1, win.tabs.length);
        var openerId = win.tabs[0].id;

        // Try to update the tab in the window to have an opener of tab
        // of itself. This *should* fail.
        chrome.tabs.update(openerId, {openerTabId: openerId}, () => {
          chrome.test.assertLastError(
              'Cannot set a tab\'s opener to itself.');
          chrome.test.succeed();
        });
      });
    });
  }
]);

