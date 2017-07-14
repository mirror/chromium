// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const networkIds = [];

chrome.dial.onNetworkIdChanged.addListener(networkId => {
  if (networkIds.length < 2) {
    chrome.test.assertFalse(networkId == "__unknown__");
    chrome.test.assertFalse(networkId == "__disconnected__");
    chrome.test.assertFalse(networkId.length == 0);
  }
  networkIds.push(networkId);
  chrome.test.sendMessage("continue");

  if (networkIds.length == 2) {
    chrome.test.assertFalse(networkIds[0] == networkIds[1]);
  } else if (networkIds.length == 3) {
    chrome.test.assertTrue(networkId == "__disconnected__");
  } else if (networkIds.length == 4) {
    chrome.test.assertTrue(networkId == "__unknown__");
    chrome.test.succeed();
  }
});

chrome.test.notifyPass();
