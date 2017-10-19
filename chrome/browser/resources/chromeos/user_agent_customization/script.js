// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Invokes extension that customizes User-Agent. */

window.addEventListener('load', function() {
  chrome.runtime.sendMessage('START');
});
