// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function $(id) { return document.getElementById(id); }

document.addEventListener('DOMContentLoaded', function() {
  if (cr.isChromeOS) {
    var keyboardUtils = document.createElement('script');
    keyboardUtils.src = 'chrome://credits/keyboard_utils.js';
    document.body.appendChild(keyboardUtils);
  }

  document.getElementById('print-link').hidden = false;
  $('print-link').onclick = function() {
    window.print();
    return false;
  };
});
