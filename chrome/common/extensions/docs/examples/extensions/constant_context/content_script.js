// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.storage.local.get(['words'], function(object) {
  object.words.forEach(function(word) {
    $('p:contains(' + word + ')')
        .css('background-color', '#f7d68f');
    $('span:contains(' + word + ')')
        .css('background-color', '#f7d68f');
    $('h1:contains(' + word + ')')
        .css('background-color', '#8ae2a0');
    $('h2:contains(' + word + ')')
        .css('background-color', '#8ae2a0');
    $('h3:contains(' + word + ')')
        .css('background-color', '#8ae2a0');
    $('li:contains(' + word + ')')
        .css('background-color', '#89b1ed');
    $('th:contains(' + word + ')')
        .css('background-color', '#89b1ed');
    $('td:contains(' + word + ')')
        .css('background-color', '#89b1ed');
  });
});
