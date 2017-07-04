// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* eslint-disable no-restricted-properties */
function $(id) { return document.getElementById(id); }
/* eslint-enable no-restricted-properties */

function onToggleKeyPress(e) {
  if (e.keyCode == 13) {
    var checkbox = e.currentTarget.previousElementSibling;
    checkbox.checked = !checkbox.checked;
  }
}

document.addEventListener('DOMContentLoaded', function() {
  if (cr.isChromeOS) {
    var keyboardUtils = document.createElement('script');
    keyboardUtils.src = 'chrome://credits/keyboard_utils.js';
    document.body.appendChild(keyboardUtils);
  }

  $('print-link').hidden = false;
  $('print-link').onclick = function() {
    window.print();
    return false;
  };

  // This block makes the license show/hide toggle when the Enter key is
  // pressed.
  var links = document.querySelectorAll('label');
  for (var i = 0; i < links.length; ++i) {
    links[i].onkeypress = onToggleKeyPress;
  }
});
