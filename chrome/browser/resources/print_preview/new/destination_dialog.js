// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-destination-dialog',

  properties: {
    destinations: Array,
  },

  show: function() {
    if (!this.$.dialog.open)
      this.$.dialog.showModal();
    this.changeButtonTapped = false;
  },

  /** @private */
  onCancelButtonTap_: function() {
    this.$.dialog.cancel();
  },
});
