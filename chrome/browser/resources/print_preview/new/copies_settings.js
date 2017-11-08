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
    },

    /** @private {string} */
    copiesString_: {
      type: String,
      value: '1',
    },
  },

  /**
   * @param {!KeyboardEvent} e The keyboard event
   * @private
   */
  onCopiesKeydown_: function(e) {
    if (e.key == '.' || e.key == 'e' || e.key == '-')
      e.preventDefault();
  },

  /** @private */
  onCopiesBlur_: function() {
    if (this.copiesString_ == '') {
      this.$$('.user-value').value = '1';
      this.set('copiesString_', '1');
    }
  },

  /**
   * @return {boolean} Whether copies value is invalid.
   * @private
   */
  copiesValid_: function() {
    const valid =
        this.$$('.user-value').validity.valid && this.copiesString_ != '';
    if (valid)
      this.set('model.copies', parseInt(this.copiesString_, 10));
    else if (this.copiesString_ == '')
      this.set('model.copies', 1);
    return valid;
  },

  /**
   * @return {boolean} Whether collate checkbox should be hidden.
   * @private
   */
  collateHidden_: function() {
    return !this.copiesValid_() || this.copiesString_ == '1';
  },

  /**
   * @return {boolean} Whether error message should be hidden.
   * @private
   */
  hintHidden_: function() {
    const valid = this.copiesValid_();
    this.set('model.printTicketInvalid', !valid);
    return valid || this.copiesString_ == '';
  },
});
