// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-other-options-settings',

  behaviors: [SettingsBehavior],

  observers: [
    'onHeaderFooterSettingChange_(settings.headerFooter.value)',
    'onDuplexSettingChange_(settings.duplex.value)',
    'onCssBackgroundSettingChange_(settings.cssBackground.value)',
    'onRasterizeSettingChange_(settings.rasterize.value)',
    'onSelectionOnlySettingChange_(settings.selectionOnly.value)',
  ],

  /**
   * @param {?Element} checkbox The checkbox to update
   * @param {*} value The value to update to.
   * @private
   */
  updateCheckbox_: function(checkbox, value) {
    if (!checkbox)
      return;
    checkbox.checked = /** @type {boolean} */ (value);
  },

  /** @private */
  onHeaderFooterSettingChange_: function(value) {
    this.updateCheckbox_(this.$$('#header-footer'), value);
  },

  /** @private */
  onDuplexSettingChange_: function(value) {
    this.updateCheckbox_(this.$$('#duplex'), value);
  },

  /** @private */
  onCssBackgroundSettingChange_: function(value) {
    this.updateCheckbox_(this.$$('#css-background'), value);
  },

  /** @private */
  onRasterizeSettingChange_: function(value) {
    this.updateCheckbox_(this.$$('#rasterize'), value);
  },

  /** @private */
  onSelectionOnlySettingChange_: function(value) {
    this.updateCheckbox_(this.$$('#selection-only'), value);
  },

  /** @private */
  onHeaderFooterChange_: function() {
    this.setSetting('headerFooter', this.$$('#header-footer').checked);
  },

  /** @private */
  onDuplexChange_: function() {
    this.setSetting('duplex', this.$$('#duplex').checked);
  },

  /** @private */
  onCssBackgroundChange_: function() {
    this.setSetting('cssBackground', this.$$('#css-background').checked);
  },

  /** @private */
  onRasterizeChange_: function() {
    this.setSetting('rasterize', this.$$('#rasterize').checked);
  },

  /** @private */
  onSelectionOnlyChange_: function() {
    this.setSetting('selectionOnly', this.$$('#selection-only').checked);
  },
});
