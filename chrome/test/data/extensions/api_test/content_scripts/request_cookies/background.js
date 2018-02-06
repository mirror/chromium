// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.getConfig(function(config) {
  var testUrl =
      'http://localhost:' + config.testServer.port + '/extensions/body1.html';

  // Create a tab and navigate it to a plain document. The injected content
  // script will drive the actual test.
  chrome.tabs.create({ url: testUrl });
});
