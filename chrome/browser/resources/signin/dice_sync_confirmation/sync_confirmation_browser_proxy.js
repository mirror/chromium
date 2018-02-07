// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A helper object used by the sync confirmation dialog to
 * interact with the browser.
 */

cr.define('sync.confirmation', function() {

  /** @interface */
  class SyncConfirmationBrowserProxy {
    confirm() {}
    undo() {}
    goToSettings() {}

    /** @param {!Array<number>} height */
    initializedWithSize(height) {}

    /**
     * Records that the user consented to Sync.
     * @param {!Array<!string>} description Strings that the user was presented
     *     with in the UI.
     * @param {!string} confirmation Text of the element that the user
     *     clicked on.
     */
    recordConsent(description, confirmation) {}
  }

  /** @implements {sync.confirmation.SyncConfirmationBrowserProxy} */
  class SyncConfirmationBrowserProxyImpl {
    /** @override */
    confirm() {
      chrome.send('confirm');
    }

    /** @override */
    undo() {
      chrome.send('undo');
    }

    /** @override */
    goToSettings() {
      chrome.send('goToSettings');
    }

    /** @override */
    initializedWithSize(height) {
      chrome.send('initializedWithSize', height);
    }

    /** @override */
    recordConsent(description, confirmation) {
      chrome.send('recordConsent', [description, confirmation]);
    }
  }

  cr.addSingletonGetter(SyncConfirmationBrowserProxyImpl);

  return {
    SyncConfirmationBrowserProxy: SyncConfirmationBrowserProxy,
    SyncConfirmationBrowserProxyImpl: SyncConfirmationBrowserProxyImpl,
  };
});