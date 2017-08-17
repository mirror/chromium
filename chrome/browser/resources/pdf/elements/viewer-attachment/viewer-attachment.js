// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
Polymer({
  is: 'viewer-attachment',

  properties: {
    /**
     * An attachment object, each containing a:
     * - title
     * - file
     */
    attachment: {type: Object},

    keyEventTarget: {
      type: Object,
      value: function() {
        return this.$.item;
      }
    }
  },

  behaviors: [Polymer.IronA11yKeysBehavior],

  keyBindings: {'enter': 'onEnter_', 'space': 'onSpace_'},

  onClick: function() {
    this.fire('hide-dropdowns');

    // Create a blob url for the attachment file and download it.
    var file = new File([this.attachment.file], this.attachment.title);
    var url = window.URL.createObjectURL(file);
    this.fire('save-attachment', {url: url});
  },

  onEnter_: function(e) {
    this.onClick();
  },

  onSpace_: function(e) {
    this.onClick();
    e.detail.keyboardEvent.preventDefault();
  }
});
})();
