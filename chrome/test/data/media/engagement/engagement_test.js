// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

let count = 0;

function register(tagName) {
  const elements = document.getElementsByTagName(tagName);

  for (var i = 0; i < elements.length; i++) {
    count++;

    elements[i].onplay = () => {
      count--;

      if (count == 0)
        window.document.title = "Ready";
    };
  }
}

register('video');
register('audio');
