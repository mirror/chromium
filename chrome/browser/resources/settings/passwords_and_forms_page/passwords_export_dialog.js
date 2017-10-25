// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'passwords-export-dialog' is the dialog that allows exporting
 * passwords.
 */

(function() {
'use strict';

Polymer({
  is: 'passwords-export-dialog',

  behaviors: [],

  properties: {
    /** @private */
    showPreparing_: Boolean,

    /** @private */
    showPrepared_: Boolean,

    /** @private */
    showSuccess_: Boolean,

    /** @private */
    showError_: Boolean,
  },

  /**
   * @type {?function():void}
   * @private
   */
  onPasswordsExportedListener_: null,
  /**
   * @type {?function():void}
   * @private
   */
  onPasswordsReadyForExportListener_: null,

  /** @override */
  attached: function() {
    this.$.dialog.showModal();

    var onPasswordsExportedListener = success => {
      if (success)
        this.setState_('SUCCESS');
      else
        this.setState_('ERROR');
    };
    var onPasswordsReadyForExportListener = list => {
      this.setState_('PREPARED');
    };

    this.onPasswordsExportedListener_ = onPasswordsExportedListener;
    this.onPasswordsReadyForExportListener_ = onPasswordsReadyForExportListener;

    if (!chrome.passwordsPrivate.onPasswordsExported.hasListener(
            onPasswordsExportedListener)) {
      chrome.passwordsPrivate.onPasswordsExported.addListener(
          onPasswordsExportedListener);
    }
    if (!chrome.passwordsPrivate.onPasswordsReadyForExport.hasListener(
            onPasswordsReadyForExportListener)) {
      chrome.passwordsPrivate.onPasswordsReadyForExport.addListener(
          onPasswordsReadyForExportListener);
    }

    chrome.passwordsPrivate.preparePasswordsForExport();
    this.setState_('PREPARING');
  },

  /** @override */
  ready: function() {
    this.setState_('NOT_STARTED');
  },

  /** Closes the dialog. */
  close: function() {
    chrome.passwordsPrivate.onPasswordsExported.removeListener(
        this.onPasswordsExportedListener);
    chrome.passwordsPrivate.onPasswordsReadyForExport.removeListener(
        this.onPasswordsReadyForExportListener);
    this.$.dialog.close();
  },

  /** @private */
  setState_: function(state) {
    this.showPreparing_ = state == 'PREPARING';
    this.showPrepared_ = state == 'PREPARED';
    this.showSuccess_ = state == 'SUCCESS';
    this.showError_ = state == 'ERROR';
  },

  /**
   * Fires an event that should trigger the password export process.
   * @private
   */
  onExportTap_: function() {
    chrome.passwordsPrivate.exportPasswords();
  },

  /**
   * Handler for tapping the 'done' button. Should just dismiss the dialog.
   * @private
   */
  onActionButtonTap_: function() {
    this.close();
  },
});
})();