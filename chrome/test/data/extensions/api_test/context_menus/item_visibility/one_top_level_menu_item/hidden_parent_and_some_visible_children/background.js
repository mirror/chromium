// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var callbackPass = chrome.test.callbackPass;

// Create a hidden parent item with three children. This first and third are
// hidden. The second is visible.
var id = chrome.contextMenus.create({title: "parent", visible: false},
    callbackPass());
chrome.contextMenus.create(
    {title: "child1", parentId: id, visible: false}, callbackPass());
chrome.contextMenus.create(
    {title: "child2", parentId: id, visible: true}, callbackPass());
chrome.contextMenus.create(
    {title: "child3", parentId: id, visible: false}, callbackPass());
chrome.test.notifyPass();
