// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var callbackPass = chrome.test.callbackPass;

// Create one visible menu item.
chrome.contextMenus.create({title: "item", visible: true}, callbackPass());
chrome.test.notifyPass();
