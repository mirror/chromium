// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

const kButtonColors = ['#3aa757', '#e8453c', '#f9bb2d', '#4688f1']
function constructOptions(kButtonColors) {
  let page = document.getElementById('buttonDiv');
  for (let item of kButtonColors) {
   let button = document.createElement('button');
   button.style.backgroundColor = item;
   button.setAttribute('id', item);
   button.addEventListener('click', function() {
      console.log(button.id)
      chrome.storage.sync.set({color: button.id}, function(){
        console.log("color is " + button.id)
      })
   });
   page.appendChild(button)
  }
}
constructOptions(kButtonColors)
