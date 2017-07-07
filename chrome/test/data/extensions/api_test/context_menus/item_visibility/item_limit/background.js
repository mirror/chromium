// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Create a total number of chrome.contextMenus.ACTION_MENU_TOP_LEVEL_LIMIT menu
// items.
var item1Id;
item1Id = chrome.contextMenus.create({title:'item1'}, function() {
  chrome.test.assertNoLastError();
  var item2Id = chrome.contextMenus.create({title:'item2'}, function() {
    chrome.test.assertNoLastError();
    chrome.contextMenus.create({title:'item3'}, function() {
      chrome.test.assertNoLastError();
      chrome.contextMenus.create({title:'item4'}, function() {
        chrome.test.assertNoLastError();
        chrome.contextMenus.create({title:'item5'}, function() {
          chrome.test.assertNoLastError();
          chrome.contextMenus.create({title:'item6'}, function() {
            chrome.test.assertNoLastError();
            // Update and hide the first two items in the menu.
            chrome.contextMenus.update(item1Id, {visible: false}, function() {
              chrome.test.assertNoLastError();
              chrome.contextMenus.update(item2Id, {visible: false}, function() {
                chrome.test.assertNoLastError();
                // Create and add two more items to the menu.
                chrome.contextMenus.create({title:'newItem1'}, function() {
                  chrome.test.assertNoLastError();
                  chrome.contextMenus.create({title:'newItem2'}, function() {
                    chrome.test.assertNoLastError();
                    chrome.test.notifyPass();
                  });
                });
              });
            });
          });
        });
      });
    });
  });
});






