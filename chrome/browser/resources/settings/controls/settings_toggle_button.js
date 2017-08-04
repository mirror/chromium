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
    elideLabel: {
      type: Boolean,
      reflectToAttribute: true,
    },

    /**
     * Which element will trigger state changes. Defaults to |this|.
     * @type {HTMLElement}
     */
    actionTarget: {
      type: Object,
    },
  },

  observers: [
    'onDisableOrPrefChange_(disabled, pref.*, actionTarget)',
  ],

  /** @override */
  attached: function() {
    if (!this.actionTarget) {
      this.actionTarget = this;
    }
    this.listen(this.actionTarget, 'tap', 'onLabelWrapperTap_');
  },

  /** @override */
  focus: function() {
    this.$.control.focus();
  },

  /** @private */
  onDisableOrPrefChange_: function() {
    if (this.controlDisabled_()) {
      this.actionTarget.removeAttribute('actionable');
    } else {
      this.actionTarget.setAttribute('actionable', '');
    }
  },

  /** @private */
  onLabelWrapperTap_: function() {
    if (this.controlDisabled_())
      return;

    this.checked = !this.checked;
    this.notifyChangedByUserInteraction();
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
