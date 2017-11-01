// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.commands.onCommand.addListener(function(command) {
  chrome.tabs.getSelected(null, function(tab) {
    let tabForward = tab.index + 1;
    let tabBackward = tab.index - 1;
    chrome.tabs.query({currentWindow: true}, function(tabs) {
      let tabIndexLength = tabs.length;
      if(command === "flip-tabs-forward") {
        if (tabForward === tabIndexLength) {
          chrome.tabs.query({index: 0}, function(tabStart) {
            chrome.tabs.update(tabStart[0].id, {"active":true,"highlighted":true});
          });
        } else {
          tabs.forEach(function(tab) {
            if (tab.index === tabForward) {
              chrome.tabs.update(tab.id, {"active":true,"highlighted":true});
            }
          });
        }
      } else if(command === "flip-tabs-backwards") {
        if (tabBackward < 0) {
          let lastTab = tabIndexLength-1;
          chrome.tabs.query({index: lastTab}, function(tabEnd) {
            chrome.tabs.update(tabEnd[0].id, {"active":true,"highlighted":true});
          });
        } else {
          tabs.forEach(function(tab) {
            if (tab.index === tabBackward) {
              chrome.tabs.update(tab.id, {"active":true,"highlighted":true});
            }
          });
        }
      }
    });
  })
});
