// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Claim clients to send postMessage reply to them.
// TODO(lazyboy): Is this necessary?
self.addEventListener('activate', function(e) {
  e.waitUntil(self.clients.claim());
});

chrome.tabs.onCreated.addListener(function(tab) {
  console.log('onCreated: tab.id = ' + tab.id);
  chrome.test.sendMessage('hello-newtab');
});
