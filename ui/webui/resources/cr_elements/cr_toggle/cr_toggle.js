// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'cr-toggle' is a component for showing an on/off switch.
 */
Polymer({
  is: 'cr-toggle',

  properties: {
    checked: {
      type: Boolean,
      value: false,
      reflectToAttribute: true,
    },

    disabled: {
      type: Boolean,
      value: false,
      reflectToAttribute: true,
    },
  },

  listeners: {
    'keypress': 'onKeyPress_',
  },

  /** @private */
  onTap_: function() {
    this.checked = !this.checked;
    this.fire('change', this.checked);
  },

  /** @private {!KeyboardEvent} */
  onKeyPress_: function(e) {
    if (this.disabled)
      return;

    if (e.code == 'Space' || e.code == 'Enter')
      this.onTap_();
  },
});
