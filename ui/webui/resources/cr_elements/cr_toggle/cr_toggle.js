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
      observer: 'checkedChanged_',
    },

    disabled: {
      type: Boolean,
      value: false,
      reflectToAttribute: true,
      observer: 'disabledChanged_',
    },

    tabIndex: {
      type: Number,
      value: 1,
    },
  },

  hostAttributes: {
    role: 'button',
    'aria-pressed': 'false',
  },

  listeners: {
    'keypress': 'onKeyPress_',
  },

  ready: function() {
    console.log(
        'ready', this.checked, this.disabled,
        this.getAttribute('aria-pressed'));
  },

  checkedChanged_: function() {
    console.log('checkedChanged', this.checked);
    this.setAttribute('aria-pressed', this.checked ? 'true' : 'false');
  },

  /** @private */
  disabledChanged_: function() {
    this.tabIndex = this.disabled ? -1 : 1;
  },

  /** @private */
  onTap_: function() {
    this.checked = !this.checked;
    this.fire('change', this.checked);
  },

  /** @private {!KeyboardEvent} */
  onKeyPress_: function(e) {
    if (e.code == 'Space' || e.code == 'Enter')
      this.onTap_();
  },

  // TODO(dpapad): role aria-pressed, aria-disabled, aria-label
});
