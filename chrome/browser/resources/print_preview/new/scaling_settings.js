// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-scaling-settings',

  behaviors: [SettingsBehavior],

  properties: {
    /** @type {Object} */
    documentInfo: Object,

    /** @private {string} */
    inputString_: String,

    /** @private {boolean} */
    inputValid_: Boolean,
  },

  /** @private {string} */
  lastValidScaling_: '100',

  /** @private {number} */
  fitToPageFlag_: 0,

  /** @private {boolean} */
  isInitialized_: false,

  observers: [
    'onInputChanged_(inputString_, inputValid_)',
    'onInitialized_(settings.scaling.value)'
  ],

  /**
   * Updates the input string when the setting has been initialized.
   * @private
   */
  onInitialized_: function() {
    if (this.isInitialized_)
      return;
    this.isInitialized_ = true;
    const scaling = this.getSetting('scaling');
    this.set('inputString_', scaling.value);
  },

  /**
   * Updates model.scaling and model.scalingInvalid based on the validity
   * and current value of the scaling input.
   * @private
   */
  onInputChanged_: function() {
    if (this.fitToPageFlag_ > 0) {
      this.fitToPageFlag_--;
      return;
    }
    const checkbox = this.$$('.checkbox input[type="checkbox"]');
    if (checkbox.checked && !this.documentInfo.isModifiable) {
      checkbox.checked = false;
    } else if (this.inputValid_) {
      this.lastValidScaling_ = this.inputString_;
    }
    this.setSettingValid('scaling', this.inputValid_);
  },

  /**
   * Updates scaling as needed based on the value of the fit to page checkbox.
   */
  onFitToPageChange_: function() {
    if (this.$$('.checkbox input[type="checkbox"]').checked) {
      this.fitToPageFlag_ = 2;
      this.set('inputString_', this.documentInfo.fitToPageScaling);
    } else {
      this.set('inputString_', this.lastValidScaling_);
    }
  },
});
