// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-stylus' is the settings subpage with stylus-specific settings.
 */

/** @const */ var FIND_MORE_APPS_URL = 'https://play.google.com/store/apps/' +
    'collection/promotion_30023cb_stylus_apps';

Polymer({
  is: 'settings-stylus',

  properties: {
    /** Preferences state. */
    prefs: {
      type: Object,
      notify: true,
    },

    /**
     * Note taking apps the user can pick between.
     * @private {Array<!settings.NoteAppInfo>}
     */
    appChoices_: {
      type: Array,
      value: function() {
        return [];
      }
    },

    /**
     * Currently selected note taking app.
     * @private {?settings.NoteAppInfo}
     */
    selectedApp_: {
      type: Object,
      value: null,
    },

    /**
     * True if the ARC container has not finished starting yet.
     * @private
     */
    waitingForAndroid_: {
      type: Boolean,
      value: false,
    },
  },

  /**
   * @return {boolean} Whether note taking from the lock screen is supported
   *     by the selected note-taking app.
   */
  supportsLockScreen_: function() {
    return !!this.selectedApp_ &&
        this.selectedApp_.lockScreenSupport !=
        settings.NoteAppLockScreenSupport.NOT_SUPPORTED;
  },

  /**
   * @return {boolean} Whether the selected app is disallowed to handle note
   *     actions from lock screen as a result of a user policy.
   */
  disallowedOnLockScreenByPolicy_: function() {
    return !!this.selectedApp_ &&
        this.selectedApp_.lockScreenSupport ==
        settings.NoteAppLockScreenSupport.NOT_ALLOWED_BY_POLICY;
  },

  /**
   * @return {boolean} Whether the selected app is enabled as a note action
   *     handler on the lock screen.
   */
  lockScreenSupportEnabled_: function() {
    return !!this.selectedApp_ &&
        this.selectedApp_.lockScreenSupport ==
        settings.NoteAppLockScreenSupport.ENABLED;
  },

  /** @private {?settings.DevicePageBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.DevicePageBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    this.browserProxy_.setNoteTakingAppsUpdatedCallback(
        this.onNoteAppsUpdated_.bind(this));
    this.browserProxy_.requestNoteTakingApps();
  },

  /**
   * Finds note app info with the provided app id.
   * @param {!string} id
   * @return {?settings.NoteAppInfo}
   * @private
   */
  findApp_: function(id) {
    return this.appChoices_.find(function(app) {
      return app.value == id;
    }) ||
        null;
  },

  /**
   * Toggles whether the selected app is enabled as a note action handler on
   * lock screen. If the app is enabled on the lock screen, the app will be
   * disabled; if the app is disabled on the lock screen it will be enabled.
   * Should not be called if the app does not support note taking on lock
   * screen, nor if the app is not allowed on lock screen by policy.
   */
  toggleLockScreenSupport_: function() {
    assert(!!this.selectedApp_);
    if (this.selectedApp_.lockScreenSupport !=
            settings.NoteAppLockScreenSupport.ENABLED &&
        this.selectedApp_.lockScreenSupport !=
            settings.NoteAppLockScreenSupport.SUPPORTED) {
      return;
    }

    var enable = this.selectedApp_.lockScreenSupport ==
        settings.NoteAppLockScreenSupport.SUPPORTED;
    this.browserProxy_.setNoteTakingAppEnabledOnLockScreen(
        this.selectedApp_.value, enable);
  },

  /** @private */
  onSelectedAppChanged_: function() {
    var app = this.findApp_(this.$.menu.value);

    if (app && !app.preferred) {
      this.browserProxy_.setPreferredNoteTakingApp(app.value);
    } else {
      this.selectedApp_ = app;
    }
  },

  /**
   * @param {Array<!settings.NoteAppInfo>} apps
   * @param {boolean} waitingForAndroid
   * @private
   */
  onNoteAppsUpdated_: function(apps, waitingForAndroid) {
    this.waitingForAndroid_ = waitingForAndroid;
    this.appChoices_ = apps;

    // Wait until app selection UI is updated before setting the selected app.
    this.async(this.onSelectedAppChanged_.bind(this));
  },

  /**
   * @param {Array<!settings.NoteAppInfo>} apps
   * @param {boolean} waitingForAndroid
   * @private
   */
  showNoApps_: function(apps, waitingForAndroid) {
    return apps.length == 0 && !waitingForAndroid;
  },

  /**
   * @param {Array<!settings.NoteAppInfo>} apps
   * @param {boolean} waitingForAndroid
   * @private
   */
  showApps_: function(apps, waitingForAndroid) {
    return apps.length > 0 && !waitingForAndroid;
  },

  /** @private */
  onFindAppsTap_: function() {
    this.browserProxy_.showPlayStore(FIND_MORE_APPS_URL);
  },
});
