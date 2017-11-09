// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-scaling-settings',

  properties: {
    /** @type {!print_preview_new.Model} */
    model: {
      type: Object,
      notify: true,
    },

    /** @private {string} */
    scalingString_: {
      type: String,
      value: '100',
    },

    /** @private {boolean} */
    scalingValid_: {
      type: Boolean,
      computed: 'computeScalingValid_(scalingString_)',
    },
  },

  /** @private {string} */
  lastValidScaling_: '100',

  /** @private {boolean} */
  fitToPageFlag_: false,

  observers: ['onScalingChanged_(scalingString_, scalingValid_)'],

  /**
   * @param {!KeyboardEvent} e The keyboard event
   * @private
   */
  onScalingKeydown_: function(e) {
    if (e.key == '.' || e.key == 'e' || e.key == '-')
      e.preventDefault();
  },

  /** @private */
  onScalingBlur_: function() {
    if (this.scalingString_ == '') {
      this.$$('.user-value').value = '100';
      this.set('scalingString_', '100');
    }
  },

  /**
   * @return {boolean} Whether scaling value represented by scalingString_ is
   *     valid.
   * @private
   */
  computeScalingValid_: function() {
    return this.$$('.user-value').validity.valid && this.scalingString_ != '';
  },

  /**
   * Updates model.scaling and model.scalingInvalid based on the validity
   * and current value of the scaling input.
   * @private
   */
  onScalingChanged_: function() {
    if (this.fitToPageFlag_) {
      this.fitToPageFlag_ = false;
      return;
    }
    const checkbox = this.$$('.checkbox input[type="checkbox"]');
    if (checkbox.checked && this.model.isPdfDocument) {
      checkbox.checked = false;
    } else if (this.scalingValid_) {
      this.lastValidScaling_ = this.scalingString_;
    }
    this.set('model.scalingInvalid', !this.scalingValid_);
  },

  /**
   * Updates scaling as needed based on the value of the fit to page checkbox.
   */
  onFitToPageTap_: function() {
    if (this.$$('.checkbox input[type="checkbox"]').checked) {
      this.$$('.user-value').value = this.model.fitToPageScaling;
      this.fitToPageFlag_ = true;
      this.scalingString_ = this.model.fitToPageScaling;
    } else {
      this.$$('.user-value').value = this.lastValidScaling_;
      this.set('scalingString_', this.lastValidScaling_);
    }
  },

  /**
   * @return {boolean} Whether error message should be hidden.
   * @private
   */
  hintHidden_: function() {
    return this.scalingValid_ || this.scalingString_ == '';
  },
});
