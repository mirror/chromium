// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// a listener to create the initial context menu items
// context menu items only need to be created at runtime.onInstalled
'use strict';
chrome.runtime.onInstalled.addListener(function() {
  chrome.storage.local.clear();
  for (let key of Object.keys(locales)) {
    chrome.contextMenus.create({
      id: key,
      title: locales[key],
      type: 'normal',
      contexts: ['selection'],
    });
  }

});

chrome.contextMenus.onClicked.addListener(function(item, tab){
  chrome.tabs.create({
        url: 'https://google.'+ item.menuItemId +'/search?q=' + item.selectionText,
        index: tab.index + 1
    });
});

chrome.storage.onChanged.addListener(function(){
  chrome.storage.sync.get(['removedContextMenuItems'], function(list) {
      list.removedContextMenuItems.forEach(function(object) {
        let contextMenuId = object
        chrome.contextMenus.remove(contextMenuId)
    })
  })
})
