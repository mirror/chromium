// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Element which shows toasts.
 */
cr.define('passwords', function() {

  var UndoManager = Polymer({
    is: 'password-undo-toast',

    properties: {
      duration: {
        type: Number,
        value: 0,
      },

      /** @private */
      open_: {
        type: Boolean,
        observer: 'openChanged_',
        value: false,
      },

      /** @private */
      showUndo_: Boolean,
    },

    /** @private {number|null} */
    hideTimeoutId_: null,

    /** @override */
    attached: function() {
      assert(UndoManager.instance_ == null);
      UndoManager.instance_ = this;
      Polymer.RenderStatus.afterNextRender(this, function() {
        Polymer.IronA11yAnnouncer.requestAvailability();
      });
    },

    /** @override */
    detached: function() {
      UndoManager.instance_ = null;
    },

    /**
     * @param {string} label The label to display inside the toast.
     */
    show: function(label) {
      this.$.content.textContent = label;
      this.showInternal_();
    },

    /**
     * @private
     */
    showInternal_: function() {
      this.open_ = true;
      this.showUndo_ = true;
      this.fire('iron-announce', {text: this.$.content.textContent});

      if (!this.duration)
        return;

      if (this.hideTimeoutId_ != null)
        window.clearTimeout(this.hideTimeoutId_);

      this.hideTimeoutId_ = window.setTimeout(() => {
        this.hide();
        this.hideTimeoutId_ = null;
      }, this.duration);
    },

    hide: function() {
      this.open_ = false;
      // Hide the undo button to prevent it from being accessed with tab.
      this.showUndo_ = false;
    },

    /** @private */
    onUndoTap_: function() {
      // Will hide the toast.
      this.fire('command-undo');
    },

    /** @private */
    openChanged_: function() {
      this.$.toast.setAttribute('aria-hidden', String(!this.open_));
    },
  });

  /** @private {?passwords.UndoManager} */
  UndoManager.instance_ = null;

  /** @return {!passwords.UndoManager} */
  UndoManager.getInstance = function() {
    return assert(UndoManager.instance_);
  };

  return {
    UndoManager: UndoManager,
  };
});
