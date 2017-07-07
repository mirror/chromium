// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Create two menu items, which should be visible by default.
chrome.contextMenus.create({title:'item1'}, function() {
  chrome.test.assertNoLastError();
  chrome.contextMenus.create({title:'item2'}, function() {
    chrome.test.assertNoLastError();
    // Create a parent item with two child items -- one displayed and one
    // hidden. Hide the parent item from the menu.
    var id;
    id = chrome.contextMenus.create({title:'item3', visible: false},
        function() {
          chrome.test.assertNoLastError();
          chrome.contextMenus.create({title:'child1', visible: true,
              parentId : id}, function() {
            chrome.test.assertNoLastError();
            chrome.contextMenus.create({title:'child2', visible: false,
                parentId : id}, function() {
              chrome.test.assertNoLastError();
              chrome.test.notifyPass();
            });
          });
        });
  });
});
