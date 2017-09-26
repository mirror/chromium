// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// a listener to create the initial context menu items
// context menu items only need to be created at runtime.onInstalled
'use strict';
chrome.runtime.onInstalled.addListener(function() {
  chrome.storage.sync.clear();
  chrome.contextMenus.create({
    id: 'com.au',
    type: 'normal',
    title: 'Australia',
    contexts: ['selection']
  });
  chrome.contextMenus.create({
    id: 'com.br',
    type: 'normal',
    title: 'Brazil',
    contexts: ['selection']
  });
  chrome.contextMenus.create({
    id: 'ca',
    type: 'normal',
    title: 'Canada',
    contexts: ['selection']
  });
  chrome.contextMenus.create({
    id: 'cn',
    type: 'normal',
    title: 'China',
    contexts: ['selection']
  });
  chrome.contextMenus.create({
    id: 'fr',
    type: 'normal',
    title: 'France',
    contexts: ['selection']
  });
  chrome.contextMenus.create({
    id: 'it',
    type: 'normal',
    title: 'Italy',
    contexts: ['selection']
  });
  chrome.contextMenus.create({
    id: 'co.in',
    type: 'normal',
    title: 'India',
    contexts: ['selection']
  });
  chrome.contextMenus.create({
    id: 'co.jp',
    type: 'normal',
    title: 'Japan',
    contexts: ['selection']
  });
  chrome.contextMenus.create({
    id: 'com.mx',
    type: 'normal',
    title: 'Mexico',
    contexts: ['selection']
  });
  chrome.contextMenus.create({
    id: 'ru',
    type: 'normal',
    title: 'Russia',
    contexts: ['selection']
  });
  chrome.contextMenus.create({
    id: 'co.za',
    type: 'normal',
    title: 'South Africa',
    contexts: ['selection']
  });

  // stores values for context menu items for user to remove or add on options
  chrome.storage.sync.set({contextMenuItems: [{
    id: 'com.au',
    type: 'normal',
    title: 'Australia',
    contexts: ['selection']
  },
  {
    id: 'com.br',
    type: 'normal',
    title: 'Brazil',
    contexts: ['selection']
  },
  {
    id: 'ca',
    type: 'normal',
    title: 'Canada',
    contexts: ['selection']
  },
  {
    id: 'cn',
    type: 'normal',
    title: 'China',
    contexts: ['selection']
  },
  {
    id: 'fr',
    type: 'normal',
    title: 'France',
    contexts: ['selection']
  },
  {
    id: 'it',
    type: 'normal',
    title: 'Italy',
    contexts: ['selection']
  },
  {
    id: 'co.in',
    type: 'normal',
    title: 'India',
    contexts: ['selection']
  },
  {
    id: 'co.jp',
    type: 'normal',
    title: 'Japan',
    contexts: ['selection']
  },
  {
    id: 'com.mx',
    type: 'normal',
    title: 'Mexico',
    contexts: ['selection']
  },
  {
    id: 'ru',
    type: 'normal',
    title: 'Russia',
    contexts: ['selection']
  },
  {
    id: 'co.za',
    type: 'normal',
    title: 'South Africa',
    contexts: ['selection']
  }], removedContextMenuItems:
  [{
    id: 'co.uk',
    type: 'normal',
    title: 'United Kingdom',
    contexts: ['selection']
  }]})
});

chrome.contextMenus.onClicked.addListener(function(item, tab){
  console.log(item.menuItemId)
  console.log(item.selectionText)
  chrome.tabs.create({
        url: "https://google."+ item.menuItemId +"/search?q=" + item.selectionText,
        index: tab.index + 1
    });
});
