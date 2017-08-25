// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 */
Polymer({
  is: 'cr-link-row',
  extends: 'button',

  properties: {
    iconClass: String,

    label: String,

    subLabel: {
      type: String,
      /* Value used for noSubLabel attribute. */
      value: '',
    },
  },
});
