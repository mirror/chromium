// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function renovation_wikipedia() {
  // Expand sections
  elems = document.querySelectorAll(
      "div.collapsible-block,h2.collapsible-heading");

  for (i = 0; i < elems.length; ++i) {
    elems.item(i).className += " open-block";
  }

  // Remove image placeholders, load images
  placeholders = document.querySelectorAll(
      ".image > span.lazy-image-placeholder, \
       .mwe-math-element > span.lazy-image-placeholder");
  noscripts = document.querySelectorAll(
      ".image > noscript, .mwe-math-element > noscript");

  // placeholders.length should equal noscripts.length
  for (i = 0; i < placeholders.length; ++i) {
    placeholders.item(i).remove();
    innerText = noscripts.item(i).innerText;
    noscripts.item(i).outerHTML = innerText;
  }
}

var map_renovations = {
  "wikipedia" : renovation_wikipedia,
};

function run_renovations(flist) {
  for (var func_name of flist) {
    var f = map_renovations[func_name];
    f();
  }
}
