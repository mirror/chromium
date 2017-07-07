// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var permission_handler = function(e) {
  if (e.permission === 'sslCertError') {
    window.console.log('sslCertErrorNumber = ' + e.sslCertErrorNumber);
    window.console.log('url = ' + e.url);
    if (e.url.includes('https_page_allow.html')) {
      e.request.allow();
      chrome.test.sendMessage('SslCertAllowed');
    } else {
      e.request.deny();
      chrome.test.sendMessage('SslCertDenied');
    }
  } else {
    window.console.log('Unexpected permission request: ' + e.permission);
  }
};

window.loadGuest = function() {
  window.console.log('embedder.loadGuest');
  var webview = document.createElement('webview');

  webview.setAttribute('src', 'about:blank');
  webview.addEventListener('permissionrequest', permission_handler);
  webview.style.position = 'fixed';
  webview.style.left = '0px';
  webview.style.top = '0px';

  document.body.appendChild(webview);
  chrome.test.sendMessage('GuestAddedToDom');
};

var onGuestLoaded = function(e) {
  chrome.test.sendMessage('GuestLoaded');
};

window.navigateWebView = function(url) {
  var webview = document.getElementsByTagName('webview')[0];
  webview.addEventListener('loadstop', onGuestLoaded);
  window.console.log('embedder.navigateWebView: ' + url);
  webview.setAttribute('src', url);
  chrome.test.sendMessage('GuestNavigated');
};

window.onload = function() {
  chrome.test.sendMessage('EmbedderLoaded');
};
