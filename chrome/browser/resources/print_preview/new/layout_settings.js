// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-layout-settings',

  behaviors: [SettingsBehavior],

  properties: {
    selectedValue_: {
      type: String,
      observer: 'onSelectedValueChange_',
    },
  },

  observers: ['onLayoutChange_(settings.layout.value)'],

  setLayout_: false,

  onLayoutChange_: function() {
    if (this.setLayout_) {
      this.setLayout_ = false;
      return;
    }
    this.selectedValue_ =
        this.getSetting('layout').value ? 'landscape' : 'portrait';
  },

  onSelectedValueChange_: function() {
    this.setLayout_ = true;
    this.setSetting('layout', this.selectedValue_ == 'landscape');
  },
});
