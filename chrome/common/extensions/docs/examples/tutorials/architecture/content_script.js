// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

function findBackgroundColor() {
 let backgroundColor = document.head.style.backgroundColor
 console.log("Background color is " + backgroundColor)
 chrome.storage.sync.set({pageColor: backgroundColor});
};

findBackgroundColor();

chrome.runtime.onMessage.addListener(function(request, sender, sendResponse) {
  document.body.style.backgroundColor = request.color;
  console.log("Changing background color to " + request.color);
  sendResponse({done: "background color is " + request.color});
});
