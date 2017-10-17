// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  'use strict';

  /**
   * An extensions-toggle-row provides a way of having a clickable row that can
   * modify a cr-toggle, by leveraging the native "for" attribute functionality,
   * using a hidden native <input type="checkbox">.
   */
  const ExtensionsToggleRow = Polymer({
    is: 'extensions-toggle-row',

    properties: {
      checked: Boolean,
    },

    /**
     * @param {!Event}
     * @private
     */
    onLabelTap_: function(e) {
      // Ignore this event, if the interaction sequence
      // (pointerdown+pointerup) began within the cr-toggle itself.
      if (this.$.crToggle.shouldIgnoreHostTap(e))
        e.preventDefault();
    },

    /**
     * Fires when the native checkbox changes value. This happens when the user
     * clicks directly on the <label>.
     * @param {!Event} e
     * @private
     */
    onNativeChange_: function(e) {
      e.stopPropagation();

      // Sync value of native checkbox and cr-toggle and |checked|.
      this.$.crToggle.checked = this.$.native.checked;
      this.checked = this.$.native.checked;

      this.fire('change', this.checked);
    },

    /**
     * Fires
     * @param {!CustomEvent} e
     * @private
     */
    onCrToggleChange_: function(e) {
      e.stopPropagation();

      // Sync value of native checkbox and cr-toggle.
      this.$.native.checked = e.detail;

      this.fire('change', this.checked);
    },
  });

  return {ExtensionsToggleRow: ExtensionsToggleRow};
});
