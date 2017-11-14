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

  behaviors: [],

  properties: {},

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
  },

  /** @override */
  ready: function() {},

  /** Closes the dialog. */
  close: function() {
    this.$.dialog.close();
  },

  /**
   * Fires an event that should trigger the password export process.
   * @private
   */
  onExportTap_: function() {
    // TODO(cfroussios) fire event?
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