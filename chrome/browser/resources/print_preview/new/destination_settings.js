// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-destination-settings',

  properties: {
    /** @type {!print_preview.Destination} */
    destination: Object,

    /** @private {boolean} */
    loadingDestination_: {
      type: Boolean,
      value: true,
    },

    /** @type {boolean} */
    changeButtonTapped: {
      type: Boolean,
      notify: true,
    },
  },

  observers: ['onDestinationSet_(destination, destination.id)'],

  /** @private */
  onDestinationSet_: function() {
    if (this.destination && this.destination.id)
      this.loadingDestination_ = false;
  },

  onChangeButtonTap_: function() {
    this.changeButtonTapped = true;
  },
});
