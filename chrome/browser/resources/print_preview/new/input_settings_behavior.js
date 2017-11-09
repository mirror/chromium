// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview_new', function() {

  /** @polymerBehavior */
  const InputSettingsBehavior = {
    /** @private {string} */
    defaultValue_: '0',

    properties: {
      /** @private {string} */
      inputString_: {
        type: String,
      },

      /** @private {boolean} */
      inputValid_: {
        type: Boolean,
        computed: 'computeValid_(inputString_)',
      },
    },

    /** @override */
    attached: function() {
      this.$$('.user-value').value = this.defaultValue_;
      this.set('inputString_', this.defaultValue_);
    },

    /**
     * @param {!KeyboardEvent} e The keyboard event
     * @private
     */
    onInputKeydown_: function(e) {
      if (e.key == '.' || e.key == 'e' || e.key == '-')
        e.preventDefault();
    },

    /** @private */
    onInputBlur_: function() {
      if (this.inputString_ == '') {
        this.$$('.user-value').value = this.defaultValue_;
        this.set('inputString_', this.defaultValue_);
      }
    },

    /**
     * @return {boolean} Whether input value represented by inputString_ is
     *     valid.
     * @private
     */
    computeValid_: function() {
      return this.$$('.user-value').validity.valid && this.inputString_ != '';
    },

    /**
     * @return {boolean} Whether error message should be hidden.
     * @private
     */
    hintHidden_: function() {
      return this.inputValid_ || this.inputString_ == '';
    },
  };

  return {InputSettingsBehavior: InputSettingsBehavior};
});
