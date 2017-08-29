// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.diceResponseHeaderCount = 0;
window.controlResponseHeaderCount = 0;

chrome.webRequest.onHeadersReceived.addListener(function(details) {
  diceHeaderFound = false;
  headerValue = 'ValueFromExtension'
  diceResponseHeader = 'X-Chrome-ID-Consistency-Response';
  details.responseHeaders.forEach(function(v,i,a){
    if (v.name == diceResponseHeader){
      ++window.diceResponseHeaderCount;
      diceHeaderFound = true;
      v.value = headerValue;
    }
    if (v.name == 'X-Control'){
      ++window.controlResponseHeaderCount;
      v.value = headerValue;
    }
  });
  if (!diceHeaderFound) {
    details.responseHeaders.push({name: diceResponseHeader,
                                  value: headerValue});
  }
  details.responseHeaders.push({name: 'X-New-Header',
                                value: headerValue});
  return {responseHeaders:details.responseHeaders};
},
{urls: ['http://*/extensions/dice.html']},
['blocking', 'responseHeaders']);

chrome.test.sendMessage('ready');
