// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Change the background color of the current page.
 *
 * @param {string} color - The new background color.
 */
function changeBackgroundColor(color) {
  var script = 'document.body.style.backgroundColor="' + color + '"';
  chrome.tabs.executeScript({
    'code': script
  });
}

document.addEventListener('DOMContentLoaded', function() {
  var dropdown = document.getElementById("dropdown");
  dropdown.addEventListener("change", function() {
    changeBackgroundColor(dropdown.value);
  });
});
