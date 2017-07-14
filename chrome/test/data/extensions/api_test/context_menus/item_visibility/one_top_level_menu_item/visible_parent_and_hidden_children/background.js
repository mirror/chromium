// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var callbackPass = chrome.test.callbackPass;

// Create a parent item with two hidden children.
var id = chrome.contextMenus.create({title: "parent"}, callbackPass());
chrome.contextMenus.create(
  {title: "child1", parentId: id, visible: false}, callbackPass());
chrome.contextMenus.create(
  {title: "child2", parentId: id, visible: false}, callbackPass());
chrome.test.notifyPass();
