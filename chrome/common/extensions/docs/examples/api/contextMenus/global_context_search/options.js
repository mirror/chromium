// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
function checkForm() {
  chrome.storage.sync.get(['removedContextMenuItems'], function(list) {
    list.removedContextMenuItems.forEach(function(object) {
      let checkboxes = document.getElementsByTagName('input');
        for(i=0; i<checkboxes.length; i++) {
          if(checkboxes[i].value === object.id) {
            checkboxes[i].removeAttribute("checked");
          }
        }
    })
  })
}

checkForm();

document.getElementById('optionsSubmit').onclick = function() {
  let checkboxes = document.getElementsByTagName('input');
  let removed = [];
  let keep = [];
   chrome.storage.sync.get(['removedContextMenuItems'], function(list) {
     list.removedContextMenuItems.forEach(function(object) {
       for(i=0; i<checkboxes.length; i++){
         if(checkboxes[i].checked === false && checkboxes[i].value === object.id) {
           removed.push(object);
         } else if (checkboxes[i].checked === true && checkboxes[i].value === object.id) {
          keep.push(object);
          chrome.contextMenus.create({
            id: object.id,
            type: 'normal',
            title: object.title,
            contexts: ['selection']
          });
         }
       }
      chrome.storage.sync.set({removedContextMenuItems: removed, contextMenuItems: keep});
     })
   });
   chrome.storage.sync.get(['contextMenuItems'], function(list) {
     list.contextMenuItems.forEach(function(object) {
       for(i=0; i<checkboxes.length; i++){
         if(checkboxes[i].checked === false && checkboxes[i].value === object.id) {
           removed.push(object);
           let contextMenuId = object.id
           chrome.contextMenus.remove(contextMenuId)
         }else if(checkboxes[i].checked === true && checkboxes[i].value === object.id) {
           keep.push(object);
       }
     }
     chrome.storage.sync.set({removedContextMenuItems: removed, contextMenuItems: keep});
   })
 })
 window.close()
}
