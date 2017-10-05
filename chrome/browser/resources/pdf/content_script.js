// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This content script is injected by pdf.js into the tab that embeds the PDF
// viewer. It allows pdf.js to interact with the history of the tab.

chrome.runtime.onMessage.addListener(function(request, sender, sendResponse) {
  if (request.type == 'replaceHistoryState') {
    // Update the current history entry with the latest position.
    history.replaceState(request.state, '');
  } else if (request.type == 'forkHistoryState') {
    // Fork off a new history entry, so that later the user can go back to the
    // current position.
    history.pushState(history.state, '');
  } else if (request.type == 'getInitialHistoryState') {
    // Reply with the initial history.state. Re-use onpopstate for this.
    chrome.runtime.sendMessage(
        {type: 'onpopstate', url: location.href, state: history.state});
  }
});

window.addEventListener('popstate', function(e) {
  // Either:
  // a) User navigated back/forward. Tell pdf.js to scroll to the position
  //    encoded in e.state; or
  // b) User is navigating away (e.state == null). Tell pdf.js in case this is
  //    a hashchange, in which case it may need to scroll accordingly.
  chrome.runtime.sendMessage(
      {type: 'onpopstate', url: location.href, state: e.state});
});
