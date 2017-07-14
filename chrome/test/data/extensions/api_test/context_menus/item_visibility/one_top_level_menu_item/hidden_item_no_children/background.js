// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var callbackPass = chrome.test.callbackPass;

// Create a menu item.
var id = chrome.contextMenus.create({title: "item"}, callbackPass());
// Update and hide it.
chrome.contextMenus.update(id, {visible: false}, callbackPass());
chrome.test.notifyPass();
