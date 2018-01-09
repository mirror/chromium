// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var min = 1;
var max = 5;
var current = min;

// Called when the user clicks on the action.
chrome.action.onClicked.addListener(function(tab) {
  current++;
  if (current > max)
    current = min;

  chrome.action.setIcon({
    path: "icon" + current + ".png",
    tabId: tab.id
  });
  chrome.action.setTitle({
    title: "Showing icon " + current,
    tabId: tab.id
  });
  chrome.action.setBadgeText({
    text: String(current),
    tabId: tab.id
  });
  chrome.action.setBadgeBackgroundColor({
    color: [50*current,0,0,255],
    tabId: tab.id
  });

  chrome.test.notifyPass();
});

chrome.test.notifyPass();
