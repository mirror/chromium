// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var callbackPass = chrome.test.callbackPass;

// Create a total number of chrome.contextMenus.ACTION_MENU_TOP_LEVEL_LIMIT menu
// items.
var item1Id;
var item2Id;
for (var i = 1; i <= chrome.contextMenus.ACTION_MENU_TOP_LEVEL_LIMIT; i++) {
  var id = chrome.contextMenus.create({title: 'item' + i}, callbackPass());
  if (i == 1)
    item1Id = id;
  else if (i == 2)
    item2Id = id;
  else if (i == 5) {
    // Update and hide the first item in the menu.
    chrome.contextMenus.update(item1Id, {visible: false}, callbackPass());
  } else if (i == 6) {
    // Update and hide the second item in the menu.
    chrome.contextMenus.update(item2Id, {visible: false}, callbackPass());
  }
}
// Create two items to replace of the two hidden items.
chrome.contextMenus.create({title: 'newItem1'}, callbackPass());
chrome.contextMenus.create({title: 'newItem2'}, callbackPass());
chrome.test.notifyPass();
