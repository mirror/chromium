/* Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

cr.define('sync.confirmation', function() {
  'use strict';

  function initialize() {
    const syncConfirmationBrowserProxy =
        sync.confirmation.SyncConfirmationBrowserProxyImpl.getInstance();
    // Prefer using |document.body.offsetHeight| instead of
    // |document.body.scrollHeight| as it returns the correct height of the
    // even when the page zoom in Chrome is different than 100%.
    syncConfirmationBrowserProxy.initializedWithSize(
        [document.body.offsetHeight]);
  }

  function clearFocus() {
    document.activeElement.blur();
  }

  function setUserImageURL(url) {
    // TODO: remove c++'s dependency on this?'
  }

  return {
    clearFocus: clearFocus,
    initialize: initialize,
    setUserImageURL: setUserImageURL
  };
});

document.addEventListener('DOMContentLoaded', sync.confirmation.initialize);