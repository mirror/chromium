// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This content script is injected by pdf.js into the tab that embeds the PDF
// viewer. It allows pdf.js to interact with the history of the tab.

chrome.runtime.onMessage.addListener(function(request, sender, sendResponse) {
  if (request.type == 'replaceHistoryState') {
    // Update the current history entry with the latest position.
    history.replaceState(request.state, '');
  } else if (request.type == 'forkHistoryState') {
    // Fork off a new history entry since the user is clicking an internal link.
    history.pushState(history.state, '');
  } else if (request.type == 'getInitialHistoryState') {
    // Reply with the initial history.state. Re-use onpopstate for this.
    chrome.runtime.sendMessage(
        {type: 'onpopstate', url: location.href, state: history.state});
  }
});

window.addEventListener('popstate', function(e) {
  // User navigated back/forward. Tell pdf.js to scroll accordingly.
  chrome.runtime.sendMessage(
      {type: 'onpopstate', url: location.href, state: e.state});
});
