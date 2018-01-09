// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.action.setBadgeBackgroundColor({color: [255, 0, 0, 255]});
chrome.action.setBadgeText({text: 'Text'});

chrome.tabs.getSelected(null, function(tab) {
  chrome.action.setPopup({tabId: tab.id, popup: 'newPopup.html'});
  chrome.action.setTitle({tabId: tab.id, title: 'newTitle'});
  chrome.action.setBadgeBackgroundColor({
    tabId: tab.id,
    color: [0, 0, 0, 0]
  });
  chrome.action.setBadgeText({tabId: tab.id, text: 'newText'});
  chrome.test.notifyPass()
});
