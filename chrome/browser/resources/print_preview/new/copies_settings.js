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
    inputString_: String,

    /** @private {boolean} */
    inputValid_: Boolean,

    /** @private {boolean} */
    hasCollateCapability_: {
      type: Boolean,
      computed: 'computeHasCollateCapability_(model.destination.capabilities)',
    },

    /** @private {boolean} */
    hasCopiesCapability_: {
      type: Boolean,
      computed: 'computeHasCopiesCapability_(model.destination.capabilities)',
    },
  },

  observers: ['onCopiesChanged_(inputString_, inputValid_)'],

  /**
   * @return {boolean}
   * @private
   */
  computeHasCollateCapability_: function() {
    return !!this.model.destination.capabilities &&
        !!this.model.destination.capabilities.printer &&
        !!this.model.destination.capabilities.printer.collate;
  },

  /**
   * @return {boolean}
   * @private
   */
  computeHasCopiesCapability_: function() {
    return !!this.model.destination.capabilities &&
        !!this.model.destination.capabilities.printer &&
        !!this.model.destination.capabilities.printer.copies;
  },

  /**
   * Updates model.copies and model.copiesInvalid based on the validity
   * and current value of the copies input.
   * @private
   */
  onCopiesChanged_: function() {
    this.set(
        'model.copies', this.inputValid_ ? parseInt(this.inputString_, 10) : 1);
    this.set('model.copiesInvalid', !this.inputValid_);
  },

  /**
   * @return {boolean} Whether collate checkbox should be hidden.
   * @private
   */
  collateHidden_: function() {
    return !this.hasCollateCapability_ ||
        (!this.inputValid_ || parseInt(this.inputString_, 10) == 1);
  },
});
