// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

function requestProcessList() {
  chrome.send('requestProcessList');
}

function dumpProcess(pid) {
  chrome.send('dumpProcess', [pid]);
}

function returnProcessList(processList) {
  var proclist = $('proclist');
  proclist.innerText = '';
  for (var i = 0; i < processList.length; i++) {
    var row = document.createElement('div');
    row.className = 'procrow';

    var description = document.createTextNode(processList[i][1] + ' ');
    row.appendChild(description);

    var button = document.createElement('span');
    button.innerText = '[dump]';
    button.className = 'button';
    let proc_id = processList[i][0];
    button.addEventListener('click', function() {
      dumpProcess(proc_id);
    }, {passive: true});
    row.appendChild(button);

    proclist.appendChild(row);
  }
}

// Get data and have it displayed upon loading.
document.addEventListener('DOMContentLoaded', requestProcessList);

/* For manual testing.
function fakeResults() {
  returnProcessList([
    [ 11234, "Process 11234 [Browser]" ],
    [ 11235, "Process 11235 [Renderer]" ],
    [ 11236, "Process 11236 [Renderer]" ]]);
}
document.addEventListener('DOMContentLoaded', fakeResults);
*/
