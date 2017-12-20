// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-color-settings',

  behaviors: [SettingsBehavior],

  properties: {
    selectedValue_: {
      type: String,
      observer: 'onSelectedValueChange_',
    },
  },

  observers: ['onColorChange_(settings.color.value)'],

  setColor_: false,

  onColorChange_: function() {
    if (this.setColor_) {
      this.setColor_ = false;
      return;
    }
    this.selectedValue_ = this.getSetting('color').value ? 'color' : 'bw';
  },

  onSelectedValueChange_: function() {
    this.setColor_ = true;
    this.setSetting('color', this.selectedValue_ == 'color');
  },
});
