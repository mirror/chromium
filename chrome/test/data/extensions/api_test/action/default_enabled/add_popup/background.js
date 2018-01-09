// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.tabs.getSelected(null, function(tab) {
  // When the action is clicked, add a popup.
  chrome.action.onClicked.addListener(function(tab) {
    chrome.action.setPopup({
      tabId: tab.id,
      popup: 'a_popup.html'
    });
    chrome.test.notifyPass();
  });
  chrome.test.notifyPass();
});
