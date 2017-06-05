// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
'use strict';

function testNativeMojoBindingsAvailable() {
  if ('Mojo' in self && 'MojoHandle' in self && 'MojoWatcher' in self &&
      'bindInterface' in Mojo && 'writeMessage' in MojoHandle.prototype &&
      'cancel' in MojoWatcher.prototype) {
    chrome.test.succeed("Native Mojo bindings are available.");
  } else {
    chrome.test.fail("Missing native Mojo bindings.");
  }
}

chrome.test.runTests([
  testNativeMojoBindingsAvailable,
]);
