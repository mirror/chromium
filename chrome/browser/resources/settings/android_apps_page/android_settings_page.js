// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'android-settings-page' is the settings page to open Android settings.
 */

Polymer({
  is: 'settings-android-settings-page',

  /** @private {?settings.AndroidAppsBrowserProxy} */
  browserProxy_: null,

  /** @private {?WebUIListener} */
  listener_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.AndroidAppsBrowserProxyImpl.getInstance();
  },

  /** @override */
  attached: function() {
    this.listener_ = cr.addWebUIListener(
        'android-apps-info-update', this.androidAppsInfoUpdate_.bind(this));
    this.browserProxy_.requestAndroidAppsInfo();
  },

  /** @override */
  detached: function() {
    cr.removeWebUIListener(this.listener_);
  },

  /**
   * @param {AndroidAppsInfo} info
   * @private
   */
  androidAppsInfoUpdate_: function(info) {
    this.parentNode.hidden = !info.settingsAppAvailable;
  },

  /**
   * @param {Event} event
   * @private
   */
  onManageAndroidAppsKeydown_: function(event) {
    if (event.key != 'Enter' && event.key != ' ')
      return;
    this.browserProxy_.showAndroidAppsSettings(true /** keyboardAction */);
    event.stopPropagation();
  },

  /** @private */
  onManageAndroidAppsTap_: function(event) {
    this.browserProxy_.showAndroidAppsSettings(false /** keyboardAction */);
  },
});
