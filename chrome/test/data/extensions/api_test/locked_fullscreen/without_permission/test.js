// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var error_msg = 'Cannot lock window to fullscreen or close a locked fullscreen window without lockWindowFullscreenPrivate manifest permission';

openLockedFullscreenWindow = function() {
  chrome.windows.create({url: 'about:blank', state: 'locked'},
    chrome.test.callbackFail(error_msg));
};

var tests = {
  openLockedFullscreenWindow: openLockedFullscreenWindow,
};

window.onload = function() {
  chrome.test.getConfig(function(config) {
    if (config.customArg in tests)
      chrome.test.runTests([tests[config.customArg]]);
    else
      chrome.test.fail('Test "' + config.customArg + '"" doesnt exist!');
  });
};
