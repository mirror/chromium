// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'cr-toggle' is a component for showing an on/off switch.
 */
Polymer({
  is: 'cr-toggle',

  behaviors: [Polymer.PaperRippleBehavior],

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
    'aria-disabled': 'false',
    'aria-pressed': 'false',
    role: 'button',
  },

  listeners: {
    'keypress': 'onKeyPress_',
    'focus': 'onFocus_',
    'blur': 'onBlur_',
  },

  /** @private */
  checkedChanged_: function() {
    this.setAttribute('aria-pressed', this.checked ? 'true' : 'false');
  },

  /** @private */
  disabledChanged_: function() {
    this.tabIndex = this.disabled ? -1 : 1;
    this.setAttribute('aria-disabled', this.disabled ? 'true' : 'false');
  },

  /** @private */
  onFocus_: function() {
    this.ensureRipple();
    this.$$('paper-ripple').downAction();
  },

  /** @private */
  onBlur_: function() {
    this.ensureRipple();
    this.$$('paper-ripple').upAction();
  },

  /** @private */
  onTap_: function() {
    this.toggleState_(false);
  },

  /**
   * @param {boolean} fromKeyboard
   * @private
   */
  toggleState_: function(fromKeyboard) {
    this.checked = !this.checked;
    this.fire('change', this.checked);

    if (!fromKeyboard) {
      this.ensureRipple();
      this.$$('paper-ripple').simulatedRipple();
    }
  },

  /** @private {!KeyboardEvent} */
  onKeyPress_: function(e) {
    if (e.code == 'Space' || e.code == 'Enter')
      this.toggleState_(true);
  },

  // customize the element's ripple
  _createRipple: function() {
    this._rippleContainer = this.$.button;
    var ripple = Polymer.PaperRippleBehavior._createRipple();
    ripple.id = 'ink';
    ripple.setAttribute('recenters', '');
    ripple.classList.add('circle', 'toggle-ink');
    return ripple;
  },
});
