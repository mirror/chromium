// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-destination-list',

  properties: {
    name: String,

    destinations: Array,

    hasActionLink: Boolean,

    loadingDestinations: Boolean,

    hasDestinations_: {
      type: Boolean,
      computed: 'computeHasDestinations_(destinations)',
    },
  },

  computeHasDestinations_: function() {
    return this.destinations.length != 0;
  },

  onActionLinkTap_: function() {
    print_preview.NativeLayer.getInstance().managePrinters();
  },
});
