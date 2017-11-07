// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-copies-settings',

  properties: {
    /** @type {!print_preview_new.Model} */
    model: {
      type: Object,
      notify: true,
    }
  },

  /**
   * @return {boolean} Whether collate checkbox should be hidden.
   * @private
   */
  collateHidden_: function() {
    return (this.model.copies == 1 || !this.$$('.user-value').validity.valid);
  },

  /**
   * @return {boolean} Whether error message should be hidden.
   * @private
   */
  hintHidden_: function() {
    const valid = this.$$('.user-value').validity.valid;
    this.set('model.printTicketInvalid', !valid);
    console.log('Print ticket = ' + this.model.printTicketInvalid);
    return valid;
  }
});
