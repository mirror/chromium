// Copyright 2017 The Chromium Authors. All rights reserved.
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

  properties: {
    /**
     * The error that occurred while exporting.
     */
    exportErrorMessage: {
      type: String,
      value: '',
    },
  },

  /**
   * The interface for callbacks to the browser.
   * Defined in passwords_section.js
   * @type {PasswordManager}
   * @private
   */
  passwordManager_: null,

  /** @override */
  attached: function() {
    this.$.dialog.showModal();

    this.passwordManager_ = PasswordManagerImpl.getInstance();

    var onPasswordsExportCompletedListener = error => {
      if (!error) {
        this.close();
      } else {
        this.$.dialog_progress.open = false;
        this.exportErrorMessage = error;
        this.$.dialog_error.showModal();
      }
    };

    this.onPasswordsExportCompletedListener_ =
        onPasswordsExportCompletedListener;

    if (!chrome.passwordsPrivate.onPasswordsExportCompleted.hasListener(
            onPasswordsExportCompletedListener)) {
      chrome.passwordsPrivate.onPasswordsExportCompleted.addListener(
          onPasswordsExportCompletedListener);
    }
  },

  /** Closes the dialog. */
  close: function() {
    chrome.passwordsPrivate.onPasswordsExportCompleted.removeListener(
        this.onPasswordsExportCompletedListener);

    this.$.dialog.close();
    this.$.dialog_progress.close();
    this.$.dialog_error.close();
  },

  /**
   * Fires an event that should trigger the password export process.
   * @private
   */
  onExportTap_: function() {
    this.passwordManager_.exportPasswords();
    this.$.dialog.open = false;
    this.$.dialog_error.open = false;
    this.$.dialog_progress.showModal();
  },

  /**
   * Handler for tapping the 'cancel' button. Should just dismiss the dialog.
   * @private
   */
  onCancelButtonTap_: function() {
    this.close();
  },
});
})();