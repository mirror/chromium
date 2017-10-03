// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.tabs.onCreated.addListener(function(tab) {
  console.log('onCreated: tab.id = ' + tab.id + ', tab.url = ' + tab.url);
  chrome.test.sendMessage('hello-newtab');
});
