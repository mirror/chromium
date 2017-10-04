// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function checkForm(){
  let checkboxes = document.getElementsByTagName('input');
    chrome.storage.sync.get(['removedContextMenuItems'], function(list) {
      if(list.removedContextMenuItems != null) {
        for(i=0; i<checkboxes.length; i++) {
          list.removedContextMenuItems.forEach(function(object) {
            if(object === checkboxes[i].name) {
              checkboxes[i].checked = false
            }
          })
        }
      } else {
        return null
      }
    })
}

function createForm() {
        let form = document.getElementById('form');
        for (let key of Object.keys(locales)) {
          let div = document.createElement('div');
          let checkbox = document.createElement('input');
          checkbox.type = 'checkbox';
          checkbox.checked = true;
          checkbox.name = key;
          checkbox.value = locales[key];
          let span = document.createElement('span');
          span.textContent = locales[key];
          div.appendChild(checkbox);
          div.appendChild(span);
          form.appendChild(div);
        }
    checkForm()
}

createForm();

document.getElementById('optionsSubmit').onclick = function() {
  let checkboxes = document.getElementsByTagName('input');
  let removed = [];
    for(i=0; i<checkboxes.length; i++) {
      if(checkboxes[i].checked === false) {
        removed.push(checkboxes[i].name);
         } else if (checkboxes[i].checked === true) {
           chrome.contextMenus.create({
             id: checkboxes[i].name,
             type: 'normal',
             title: checkboxes[i].value,
             contexts: ['selection']
           });
          }
        }
    chrome.storage.sync.set({removedContextMenuItems: removed});
    window.close()
}
