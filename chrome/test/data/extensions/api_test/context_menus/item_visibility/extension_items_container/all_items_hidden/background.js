// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var callbackPass = chrome.test.callbackPass;

// Create 3 items that are all hidden.
chrome.contextMenus.create({title: "item1", visible: false}, callbackPass());
chrome.contextMenus.create({title: "item2", visible: false}, callbackPass());
chrome.contextMenus.create({title: "item3", visible: false}, callbackPass());
chrome.test.notifyPass();
