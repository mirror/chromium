// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var newAdapterName = 'Dome';

function testSetAdapterState() {
  chrome.bluetooth.getAdapterState(function(state) {
    chrome.test.assertNoLastError();
    chrome.test.assertFalse(state.powered);
    chrome.test.assertTrue(state.name != newAdapterName);
    setAdapterState();
  });
}

function setAdapterState() {
  var newState = {
    name: newAdapterName,
    powered: true,
    discoverable: true,
    is_user_pref: true
  };

  chrome.bluetoothPrivate.setAdapterState(newState, function() {
    chrome.test.assertNoLastError();
    if (chrome.runtime.lastError)
      chrome.test.fail(chrome.runtime.lastError);
    checkFinalAdapterState();
  });
}

var adapterStateSet = false;
function checkFinalAdapterState() {
  chrome.bluetooth.getAdapterState(function(state) {
    chrome.test.assertNoLastError();
    chrome.test.assertTrue(state.powered);
    chrome.test.assertTrue(state.name == newAdapterName);
    if (!adapterStateSet) {
      adapterStateSet = true;
      setAdapterState();
    } else {
      chrome.test.sendMessage('done', chrome.test.succeed);
    }
  });
}

chrome.test.runTests([ testSetAdapterState ]);
