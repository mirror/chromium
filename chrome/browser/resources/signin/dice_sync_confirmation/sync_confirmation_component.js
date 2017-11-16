/* Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

cr.define('sync.confirmation', function() {
  'use strict';

  Polymer({
    is: 'sync-confirmation',

    listeners: {
      // This is necessary since the settingsLink element is inserted by i18n.
      'settingsLink.tap': 'onGoToSettings_'
    },

    syncConfirmationBrowserProxy_: null,
    boundKeyDownHandler_: null,

    /** @override */
    attached: function() {
      this.syncConfirmationBrowserProxy_ = sync.confirmation.SyncConfirmationBrowserProxyImpl.getInstance();
      this.boundKeyDownHandler_ = this.onKeyDown_.bind(this);
      document.addEventListener('keydown', this.boundKeyDownHandler_);
    },

    /** @override */
    detached: function() {
      document.removeEventListener('keydown', this.boundKeyDownHandler_);
    },

    /** @private */
    onConfirm_: function() {
      this.syncConfirmationBrowserProxy_.confirm();
    },

    /** @private */
    onUndo_: function() {
      this.syncConfirmationBrowserProxy_.undo();
    },

    /** @private */
    onGoToSettings_: function() {
      this.syncConfirmationBrowserProxy_.goToSettings();
    },

    /** @private */
    onKeyDown_: function(e) {
      if (e.key == 'Enter' && !/^(A|PAPER-BUTTON)$/.test(e.path[0].tagName)) {
        this.onConfirm();
        e.preventDefault();
      }
    },
  });
});
