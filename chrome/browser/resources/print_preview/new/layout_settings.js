// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-layout-settings',

  behaviors: [SettingsBehavior],

  observers: ['onLayoutSettingChange_(settings.layout.value)'],

  onLayoutSettingChange_: function() {
    this.$$('select').value =
        this.getSetting('layout').value ? 'landscape' : 'portrait';
  },

  onChange_: function() {
    this.setSetting('layout', this.$$('select').value == 'landscape');
  },
});
