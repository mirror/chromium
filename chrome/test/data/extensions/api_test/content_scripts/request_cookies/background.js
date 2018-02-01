// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.getConfig(function(config) {
  var test_url =
      "http://localhost:PORT/extensions/body1.html"
          .replace(/PORT/, config.testServer.port);

  // Create a tab and navigate it to a plain document. The injected content
  // script will drive the actual test.
  chrome.tabs.create({ url: test_url });
});
