// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('extensions');

cr.define('extensions', function() {
  /** @interface */
  class ToolbarDelegate {
    /**
     * Toggles whether or not the profile is in developer mode.
     * @param {boolean} inDevMode
     */
    setProfileInDevMode(inDevMode) {}

    /**
     * Opens the dialog to load unpacked extensions.
     * @return {!Promise}
     */
    loadUnpacked() {}

    /** Updates all extensions. */
    updateAllExtensions() {}
  }

  const Toolbar = Polymer({
    is: 'extensions-toolbar',

    behaviors: [I18nBehavior],

    properties: {
      /** @type {extensions.ToolbarDelegate} */
      delegate: Object,

      inDevMode: {
        type: Boolean,
        value: false,
      },

      isGuest: Boolean,

      // <if expr="chromeos">
      kioskEnabled: Boolean,
      // </if>

      /** @private */
      expanded_: {
        type: Boolean,
        value: false,
      },
    },

    hostAttributes: {
      role: 'banner',
    },

    /** @override */
    ready: function() {
      if (this.inDevMode) {
        this.updateDrawer_(true);
      }
    },

    /**
     * @param {!Event} event
     * @param {boolean} isChecked
     * @private
     */
    onDevModeToggleChange_: function(event, isChecked) {
      this.updateDrawer_(isChecked);
    },

    /**
     * @param {boolean} expand Whether to expand the drawer.
     * @private
     */
    updateDrawer_: function(expand) {
      const drawer = this.$.devDrawer;
      if (drawer.hidden) {
        drawer.hidden = false;
        // Requesting the offsetTop will cause a reflow (to account for hidden).
        /** @suppress {suspiciousCode} */ drawer.offsetTop;
      }
      listenOnce(drawer, 'transitionend', () => {
        this.delegate.setProfileInDevMode(this.$['dev-mode'].checked);
        this.inDevMode = expand;
        this.$.devDrawer.hidden = !expand;
      });
      const height = this.$['button-strip'].scrollHeight;
      drawer.style.height = expand ? `${height}px` : '0';
      this.$['button-strip'].style.top = expand ? '0' : `-${height}px`;
      this.expanded_ = expand;
    },

    /** @private */
    onLoadUnpackedTap_: function() {
      this.delegate.loadUnpacked().catch(loadError => {
        this.fire('load-error', loadError);
      });
    },

    /** @private */
    onPackTap_: function() {
      this.fire('pack-tap');
    },

    // <if expr="chromeos">
    /** @private */
    onKioskTap_: function() {
      this.fire('kiosk-tap');
    },
    // </if>

    /** @private */
    onUpdateNowTap_: function() {
      this.delegate.updateAllExtensions();
    },
  });

  return {
    Toolbar: Toolbar,
    ToolbarDelegate: ToolbarDelegate,
  };
});
