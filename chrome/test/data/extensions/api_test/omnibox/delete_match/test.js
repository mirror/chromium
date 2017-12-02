// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.omnibox.onInputChanged.addListener(
  function(text, suggest) {
    chrome.test.log('onInputChanged: ' + text);
    if(text == 'd') {
      suggest([
        {content: 'n1', description: 'des1', deletable: true},
        {content: 'n2', description: 'des2', deletable: false},
      ]);
    }
  });

chrome.omnibox.onDeleteSuggestion.addListener(
  function(text) {
    chrome.test.sendMessage('onDeleteSuggestion: ' + text);
  });

// Now we wait for the input events to fire.
chrome.test.notifyPass();
