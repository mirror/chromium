// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-color-settings',

  properties: {
    /** @type {!print_preview_new.Model} */
    model: Object,

    /** @private {boolean} */
    hasColorCapability_: {
      type: Boolean,
      computed: 'computeHasColorCapability_(model.destination.capabilities)',
    },
  },

  /**
   * @private {!Array<string>} List of capability types considered color.
   * @const
   */
  COLOR_TYPES_: ['STANDARD_COLOR', 'CUSTOM_COLOR'],

  /**
   * @private {!Array<string>} List of capability types considered monochrome.
   * @const
   */
  MONOCHROME_TYPES_: ['STANDARD_MONOCHROME', 'CUSTOM_MONOCHROME'],

  /**
   * @return {boolean}
   * @private
   */
  computeHasColorCapability_: function() {
    const capabilities = this.model.destination.capabilities;
    if (!capabilities || !capabilities.printer || !capabilities.printer.color) {
      return false;
    }
    let hasColor = false;
    let hasMonochrome = false;
    capabilities.printer.color.option.forEach(option => {
      if (!option.type)
        return;
      hasColor = hasColor || (this.COLOR_TYPES_.indexOf(option.type) >= 0);
      hasMonochrome =
          hasMonochrome || (this.MONOCHROME_TYPES_.indexOf(option.type) >= 0);
    });
    return hasColor && hasMonochrome;
  }
});
