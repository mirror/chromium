/* Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

cr.define('signin.dice', function() {
  'use strict';

  function initialize() {
    $('enableSyncButton').addEventListener('click', onEnableSync);
    if (loadTimeData.getBoolean('isSignedIn')) {
      $('syncEnabledSection').hidden = false;
      $('syncDisabledSection').hidden = true;
      $('enableSyncButton').hidden = true;
      $('disableSyncButton').hidden = false;
    } else {
      $('syncEnabledSection').hidden = true;
      $('syncDisabledSection').hidden = false;
      $('disableSyncButton').hidden = true;
      if (loadTimeData.getBoolean('isSyncAllowed')) {
        $('enableSyncButton').hidden = false;
      } else {
        $('enableSyncButton').hidden = true;
      }
    }
  }

  function onEnableSync(e) {
    chrome.send('enableSync');
  }

  return {
    initialize: initialize,
  };
});

document.addEventListener('DOMContentLoaded', signin.dice.initialize);
