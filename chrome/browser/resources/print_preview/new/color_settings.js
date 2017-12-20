// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-color-settings',

  behaviors: [SettingsBehavior],

  observers: ['onColorSettingChange_(settings.color.value)'],

  onColorSettingChange_: function() {
    this.$$('select').value = this.getSetting('color').value ? 'color' : 'bw';
  },

  onChange_: function() {
    this.setSetting('color', this.$$('select').value == 'color');
  },
});
