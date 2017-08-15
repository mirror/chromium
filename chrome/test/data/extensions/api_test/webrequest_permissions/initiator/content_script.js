// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.runtime.onMessage.addListener(
  function(request, sender, sendResponse) {
    var req = new XMLHttpRequest();
    req.open('GET', request.url, true);
    req.send(null);
  });

chrome.runtime.sendMessage('injected');
