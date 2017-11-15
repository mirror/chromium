// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

console.warn('Script');
chrome.runtime.onConnect.addListener(function listener(port) {
  console.warn('on connect');
  port.onMessage.addListener((message) => {
    console.warn('on port message');
    chrome.test.assertEq('background page', message);
    port.postMessage('content script');
  });
  chrome.runtime.onConnect.removeListener(listener);
});

chrome.runtime.onMessage.addListener(
    function listener(message, sender, sendResponse) {
  console.warn('on message');
  chrome.test.assertEq('async bounce', message);
  chrome.runtime.onMessage.removeListener(listener);
  // Respond asynchronously.
  setTimeout(() => { sendResponse('bounced'); }, 0);
  // When returning a result asynchronously, the listener must return true -
  // otherwise the channel is immediately closed.
  return true;
});

console.warn('Sending message');
chrome.runtime.sendMessage('startFlow', function(response) {
  console.warn('Sent message');
  chrome.test.assertEq('started', response);
});
