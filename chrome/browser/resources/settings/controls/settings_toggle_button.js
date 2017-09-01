// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * `settings-toggle-button` is a toggle that controls a supplied preference.
 */
Polymer({
  is: 'settings-toggle-button',

  behaviors: [SettingsBooleanControlBehavior],

  properties: {
    /**
     * Hide label when toggle is visually labeled elsewhere.
     * |label| is still needed for a11y.
     */
    hideLabel: Boolean,

    elideLabel: {
      type: Boolean,
      reflectToAttribute: true,
    },
  },

  listeners: {
    'tap': 'onHostTap_',
  },

  observers: [
    'onDisableOrPrefChange_(disabled, pref.*)',
  ],

  /** @override */
  focus: function() {
    this.$.control.focus();
  },

  /**
   * TODO(hcarmona): Set label everywhere for a11y and remove this function.
   * @return {boolean}
   */
  showLabel_: function() {
    return this.label && !this.hideLabel;
  },

  /**
   * Handle taps directly on the toggle (see: onLabelWrapperTap_ for non-toggle
   * taps).
   * @param {!Event} e
   * @private
   */
  onToggleTap_: function(e) {
    // Stop the event from propagating to avoid firing two 'changed' events.
    e.stopPropagation();
  },

  /** @private */
  onDisableOrPrefChange_: function() {
    if (this.controlDisabled_()) {
      this.removeAttribute('actionable');
    } else {
      this.setAttribute('actionable', '');
    }
  },

  /**
   * Handle non-toggle button taps (see: onToggleTap_ for toggle taps).
   * @param {!Event} e
   * @private
   */
  onHostTap_: function(e) {
    // Stop the event from propagating to avoid firing two 'changed' events.
    e.stopPropagation();
    if (this.controlDisabled_())
      return;

    this.checked = !this.checked;
    this.notifyChangedByUserInteraction();
    this.fire('change');
  },

  /**
   * TODO(scottchen): temporary fix until polymer gesture bug resolved. See:
   * https://github.com/PolymerElements/paper-slider/issues/186
   * @private
   */
  resetTrackLock_: function() {
    Polymer.Gestures.gestures.tap.reset();
  },
});
