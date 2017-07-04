// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Create a total number of chrome.contextMenus.ACTION_MENU_TOP_LEVEL_LIMIT menu
// items.
console.log("maluma");
var item1Id;
item1Id = chrome.contextMenus.create({"title":"item1"});
var item2Id = chrome.contextMenus.create({"title":"item2"});
console.log("Created 2");
chrome.contextMenus.create({"title":"item3"});
chrome.contextMenus.create({"title":"item4"});
chrome.contextMenus.create({"title":"item5"});
chrome.contextMenus.create({"title":"item6"});

// Update and hide the first two items in the menu.
chrome.contextMenus.update(item1Id, {visible: false});
chrome.contextMenus.update(item2Id, {visible: false});

// Create and add two more items to the menu.
chrome.contextMenus.create({title: "newItem1"});
chrome.contextMenus.create({title: "newItem2"}, chrome.test.callbackPass());
