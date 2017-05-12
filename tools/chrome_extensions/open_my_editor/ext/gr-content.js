// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// OME for the chromium gerrit codereview site.

let clicked_element = null;

document.addEventListener('contextmenu', (event) => {
  clicked_element = event.target;
});

function handleGerritFilePaths(request, sendResponse) {
  let files = [];

  let elements = document.querySelectorAll('.file-row');
  for (var i = 0; i < elements.length; ++i) {
    let path = elements[i].getAttribute('data-path').split(' ')[0];
    if (!path || !path.length || ~path.indexOf('COMMIT_MSG'))
      continue;  // Commit message is in the file list.
    let stat = elements[i].querySelector('.status');
    if (!stat.textContent || stat.textContent.trim() == 'D')
      continue;  // File status D, it is being deleted.
    files.push(path);
  }

  sendResponse({files: files});
}

function handleGerritFile(request, sendResponse) {
  let file = clicked_element.getAttribute('title').split(' ')[0];
  if (file && file.length)
    sendResponse({file: file});
}

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  if (clicked_element && request == 'getFiles')
    handleGerritFilePaths(request, sendResponse);
  else if (clicked_element && request == 'getFile')
    handleGerritFile(request, sendResponse);
  clicked_element = null;
});
