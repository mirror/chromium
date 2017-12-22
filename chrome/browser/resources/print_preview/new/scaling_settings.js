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

  observers: [
    'onFitToPageSettingChange_(settings.fitToPage.value, ' +
        'documentInfo.fitToPageScaling)',
    'onInputChanged_(inputString_, inputValid_, settings.fitToPage.available)',
    'onScalingSettingChanged_(settings.scaling.value)',
  ],

  /** @private */
  onFitToPageSettingChange_: function() {
    this.$$('#fit-to-page-checkbox').checked =
        this.getSetting('fitToPage').value;
    if (!this.getSetting('fitToPage').value &&
        this.getSetting('scaling').valid) {
      // Fit to page is no longer checked. Update the display and scaling.
      this.setSetting('scaling', this.lastValidScaling_);
      this.inputString_ = this.lastValidScaling_;
    } else if (this.getSetting('fitToPage').value) {
      this.setSettingValid('scaling', true);
      // If scaling is currently invalid, this will trigger 2 changes, one to
      // inputValid_ and one to inputString_.
      this.fitToPageFlag_ = this.inputValid_ ? 1 : 2;
      this.inputString_ = this.documentInfo.fitToPageScaling;
    }
  },

  /**
   * Updates the input string when scaling setting is set.
   * @private
   */
  onScalingSettingChanged_: function() {
    // Update last valid scaling and ensure input string matches.
    this.lastValidScaling_ =
        /** @type {string} */ (this.getSetting('scaling').value);
    this.inputString_ = this.lastValidScaling_;
  },

  /**
   * Updates scaling and fit to page settings based on the validity and current
   * value of the scaling input.
   * @private
   */
  onInputChanged_: function() {
    const fitToPage = this.getSetting('fitToPage').value;
    if (fitToPage && !this.fitToPageFlag_) {
      // User modified scaling while fit to page was checked. Uncheck fit to
      // page.
      if (this.inputValid_)
        this.lastValidScaling_ = this.inputString_;
      else
        this.setSettingValid('scaling', false);
      this.$$('#fit-to-page-checkbox').checked = false;
      this.setSetting('fitToPage', false);
    } else if (fitToPage) {
      // Fit to page was unchecked and scaling changed as a result.
      this.fitToPageFlag_--;
    } else {
      // User modified scaling while fit to page was not checked or
      // scaling setting was set.
      this.setSettingValid('scaling', this.inputValid_);
      if (this.inputValid_)
        this.setSetting('scaling', this.inputString_);
    }
  },

  /** @private */
  onFitToPageChange_: function() {
    this.setSetting('fitToPage', this.$$('#fit-to-page-checkbox').checked);
  },
});
