// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.dial.getNetworkId(networkId => {
  chrome.test.assertFalse(networkId == "__disconnected__");
  chrome.test.assertFalse(networkId == "__unknown__");
  chrome.test.assertFalse(networkId.length == 0);
  chrome.test.succeed();
});

chrome.test.notifyPass();
