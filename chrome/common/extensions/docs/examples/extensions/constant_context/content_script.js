// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.storage.local.get(['words'], function(object) {
  var regExp = new RegExp('\\b(' + object.words.join('|') + ')\\b');
  object.words.forEach.call(
    document.querySelectorAll('p, span, li, td, h1, h2, h3, td, th'),
    function(element) {
       if ((element.tagName == 'P' || element.tagName == 'SPAN')
           && (regExp.test(element.textContent))) {
              element.style.backgroundColor = '#f7d68f';
      } else if ((element.tagName == 'LI' || element.tagName == 'TD')
                 && (regExp.test(element.textContent))) {
                    element.style.backgroundColor = '#89b1ed';
      } else if ((element.tagName == 'H1' || element.tagName == 'H2' ||
                  element.tagName == 'H3' || element.tagName == 'TH')
                  && (regExp.test(element.textContent))) {
                     element.style.backgroundColor = '#8ae2a0';
      }
    }
  )
});
