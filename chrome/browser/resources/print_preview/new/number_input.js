// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-number-input',

  properties: {
    initValue: String,

    maxValue: Number,

    minValue: Number,

    inputLabel: String,

    hintMessage: String,

    inputString: {
      type: String,
      notify: true,
    },

    inputValid: {
      type: Boolean,
      notify: true,
    }
  },

  defaultValue: '0',

  ready: function() {
    this.defaultValue = this.initValue;
  },

  behaviors: [print_preview_new.InputSettingsBehavior],
});
