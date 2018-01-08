// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Test that we can change various properties of the action.
// The C++ verifies.
chrome.tabs.getSelected(null, function(tab) {
  chrome.action.enable(tab.id);
  chrome.action.setTitle({title: "Modified", tabId: tab.id});

  chrome.test.notifyPass();
});
