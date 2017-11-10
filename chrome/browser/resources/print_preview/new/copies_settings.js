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
  },

  /** @type {string} Overrides value in InputSettingsBehavior. */
  defaultValue: '1',

  behaviors: [print_preview_new.InputSettingsBehavior],

  observers: ['onCopiesChanged_(inputString, inputValid)'],

  /**
   * Updates model.copies and model.copiesInvalid based on the validity
   * and current value of the copies input.
   * @private
   */
  onCopiesChanged_: function() {
    this.set(
        'model.copies', this.inputValid ? parseInt(this.inputString, 10) : 1);
    this.set('model.copiesInvalid', !this.inputValid);
  },

  /**
   * @return {boolean} Whether collate checkbox should be hidden.
   * @private
   */
  collateHidden_: function() {
    return !this.inputValid || parseInt(this.inputString, 10) == 1;
  },
});
