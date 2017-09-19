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
      value: 0,
    },
  },

  hostAttributes: {
    'aria-disabled': 'false',
    'aria-pressed': 'false',
    role: 'button',
  },

  listeners: {
    'pointerdown': 'onPointerDown_',
    'pointerup': 'onPointerUp_',
    'keypress': 'onKeyPress_',
    'focus': 'onFocus_',
    'blur': 'onBlur_',
  },

  /** @private {?Function} */
  boundPointerMove_: null,

  /** @private {boolean} */
  pointerMoveFired_: false,

  /** @override */
  ready: function() {
    this.boundPointerMove_ = (e) => {
      // Prevent unwanted text selection to occur while moving the pointer, this
      // is important.
      e.preventDefault();
      this.pointerMoveFired_ = true;

      var diff = e.clientX - this.pointerDownX_;
      var shouldToggle = Math.abs(diff) > 4 &&
          ((diff < 0 && this.checked) || (diff > 0 && !this.checked));
      if (shouldToggle)
        this.toggleState_(false);
    };
  },

  /** @private */
  checkedChanged_: function() {
    this.setAttribute('aria-pressed', this.checked ? 'true' : 'false');
  },

  /** @private */
  disabledChanged_: function() {
    this.tabIndex = this.disabled ? -1 : 0;
    this.setAttribute('aria-disabled', this.disabled ? 'true' : 'false');
  },

  /** @private */
  onFocus_: function() {
    this.ensureRipple();
    this.$$('paper-ripple').holdDown =
        cr.ui.FocusOutlineManager.forDocument(document).visible;
  },

  /** @private */
  onBlur_: function() {
    this.ensureRipple();
    this.$$('paper-ripple').holdDown = false;
  },

  /** @private */
  onPointerUp_: function() {
    if (!this.pointerMoveFired_)
      this.toggleState_(false);
    this.removeEventListener('pointermove', this.boundPointerMove_);
  },

  /** @private {!PointerEvent} */
  onPointerDown_: function(e) {
    // This is necessary to have follow up events fire on |this|, even
    // if they occur outside of its bounds.
    this.setPointerCapture(e.pointerId);
    this.pointerDownX_ = e.clientX;
    this.pointerMoveFired_ = false;
    this.addEventListener('pointermove', this.boundPointerMove_);
  },


  /**
   * @param {boolean} fromKeyboard
   * @private
   */
  toggleState_: function(fromKeyboard) {
    this.checked = !this.checked;

    if (!fromKeyboard) {
      this.ensureRipple();
      this.$$('paper-ripple').holdDown = false;
    }

    this.fire('change', this.checked);
  },

  /**
   * @param {!KeyboardEvent} e
   * @private
   */
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
