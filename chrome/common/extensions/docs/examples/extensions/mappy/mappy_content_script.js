// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Search the text nodes for a US-style mailing address.
// Return null if none is found.
let findAddress = function() {
  let found;
  let re = /(\d+\s+[':.,\s\w]*,\s*[A-Za-z]+\s*\d{5}(-\d{4})?)/m;
  let node = document.body;
  let done = false;
  while (!done) {
    done = true;
    for (let i = 0; i < node.childNodes.length; ++i) {
      let child = node.childNodes[i];
      if (child.textContent.match(re)) {
        node = child;
        found = node;
        done = false;
        break;
      }
    }
  }
  if (found) {
    let text = '';
    if (found.childNodes.length) {
      for (let i = 0; i < found.childNodes.length; ++i) {
        text += found.childNodes[i].textContent + ' ';
      }
    } else {
      text = found.textContent;
    }
    let match = re.exec(text);
    if (match && match.length) {
      console.log('found: ' + match[0]);
      let trim = /\s{2,}/g;
      let address = match[0].replace(trim, ' ')
      chrome.runtime.sendMessage({'address': address})
    } else {
      console.log('bad initial match: ' + found.textContent);
      console.log('no match in: ' + text);
    }
  }
  return null;
}

findAddress();
