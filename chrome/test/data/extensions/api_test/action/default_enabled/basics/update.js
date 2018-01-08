// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Test that we can change various properties of the browser action.
// The C++ verifies.
chrome.action.setTitle({title: "Modified"});
chrome.action.setIcon({path: "icon2.png"});
chrome.action.setBadgeText({text: "badge"});
chrome.action.setBadgeBackgroundColor({color: [255,255,255,255]});

chrome.test.notifyPass();
