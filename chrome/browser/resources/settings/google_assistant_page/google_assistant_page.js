// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-google-assistant-page' is the settings page
 * containing Google Assistant settings.
 */
Polymer({
  is: 'settings-google-assistant-page',

  behaviors: [I18nBehavior],

  properties: {
    /** Preferences state. */
    prefs: {
      type: Object,
      notify: true,
    },
  },

  /** @private {?settings.GoogleAssistantBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.GoogleAssistantBrowserProxyImpl.getInstance();
  },

  /**
   * @param {boolean} toggleValue
   * @return {string}
   * @private
   */
  getAssistantOnOffLabel_: function(toggleValue) {
    return this.i18n(toggleValue ? 'googleAssistantOn' : 'googleAssistantOff');
  },

  /**
   * @param {boolean} toggleValue
   * @return {string}
   * @private
   */
  getAssistantOnOffClass_: function(toggleValue) {
    return toggleValue ? 'highlighted-on' : 'highlighted-off';
  },

  /**
   * @param {!Event} event
   * @private
   */
  onGoogleAssistantEnableChange_: function(event) {
    this.browserProxy_.setGoogleAssistantEnabled(
        !!this.prefs.settings.voice_interaction.enabled.value);
  },

  /**
   * @param {!Event} event
   * @private
   */
  onGoogleAssistantContextEnableChange_: function(event) {
    this.browserProxy_.setGoogleAssistantContextEnabled(
        !!this.prefs.settings.voice_interaction.context.enabled.value);
  },

  /**
   * @param {!Event} event
   * @private
   */
  onGoogleAssistantSettingsTapped_: function(event) {
    this.browserProxy_.showGoogleAssistantSettings();
  },
});
