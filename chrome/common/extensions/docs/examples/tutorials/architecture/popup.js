// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

let displayNew = document.getElementById('displayNew')
chrome.storage.sync.get('color', function(data) {
  displayNew.style.backgroundColor = data.color;
  displayNew.setAttribute('value', data.color);
})

displayNew.onclick = function(element) {
  let color = element.target.value
  chrome.tabs.query({active: true, currentWindow: true}, function(tabs) {
    chrome.tabs.sendMessage(tabs[0].id, {color: color}, function(response) {
      console.log(response.done);
    });
  });
}

let displayDefault = document.getElementById('displayDefault')
displayDefault.onclick = function(element) {
  chrome.storage.sync.get('pageColor', function(data) {
    chrome.tabs.query({active: true,
       currentWindow: true}, function(tabs) {
      chrome.tabs.sendMessage(tabs[0].id, {color: data.pageColor},
        function(response) {
        console.log(response.done);
      });
    })
  });
}
