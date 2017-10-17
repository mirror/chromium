// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  'use strict';

  const ExtensionsToggleRow = Polymer({
    is: 'extensions-toggle-row',

    properties: {
      checked: Boolean,
    },

    onLabelTap_: function(e) {
      if (this.$.crToggle.shouldIgnoreHostTap(e))
        e.preventDefault();
    },

    onNativeChange_: function() {
      console.log('onNativeChange');
      this.$.crToggle.checked = this.$.native.checked;
      this.checked = this.$.native.checked;
      this.fire('change', this.checked);
    },

    onCrToggleChange_: function(e) {
      e.stopPropagation();
      this.$.native.checked = e.detail;

      this.fire('change', this.checked);
    },
  });

  return {ExtensionsToggleRow: ExtensionsToggleRow};
});
