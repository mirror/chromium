// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.tabs.getSelected(null, function(tab) {
  chrome.action.enable(tab.id);
  chrome.test.notifyPass();
});
