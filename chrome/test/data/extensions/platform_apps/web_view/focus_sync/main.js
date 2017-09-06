// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
'use strict';

function $(id) {
  return document.getElementById(id);
}

window.reloadWebview = function() {
    $('webview').reload();
    $('webview').focus();
};

window.addEventListener('DOMContentLoaded', function() {
  console.log('loaded');
  var webview = document.querySelector('#webview');
  webview.addEventListener('loadstop', function() {
    console.log('load stop');
    if(document.hasFocus()) {
      console.log('focus yes');
      chrome.test.sendMessage('WebViewTest.WEBVIEW_LOADED');
    } else {
      console.log('focus no');
      var wasFocused = function() {
        chrome.test.sendMessage('WebViewTest.WEBVIEW_LOADED');
        window.removeEventListener('focus', wasFocused);
      }
      window.addEventListener('focus', wasFocused);
    }
  });
  webview.focus();
  webview.src = 'focus_test.html';
  chrome.test.sendMessage('WebViewTest.LAUNCHED');
});

