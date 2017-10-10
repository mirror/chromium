// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';
// Add a listener to create the initial context menu items,
// context menu items only need to be created at runtime.onInstalled
chrome.runtime.onInstalled.addListener(function() {
  for (let key of Object.keys(countryLocales)) {
    chrome.contextMenus.create({
      id: key,
      title: countryLocales[key],
      type: 'normal',
      contexts: ['selection'],
    });
  }

});



chrome.contextMenus.onClicked.addListener(function(item, tab) {
  let url = 'https://google.'+ item.menuItemId +'/search?q=' + item.selectionText
  chrome.tabs.create({url: url, index: tab.index + 1});
});

chrome.storage.onChanged.addListener(function(list, sync) {
  let remove = list.removedContextMenu.newValue
  for (let i=0; i<remove.length; i++) {
      chrome.contextMenus.remove(remove[i]);
  }
})
