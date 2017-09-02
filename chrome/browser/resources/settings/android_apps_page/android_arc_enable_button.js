// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'android-arc-enable-button' is the button that enables Android apps.
 */
Polymer({
  is: 'android-arc-enable-button',
  behaviors: [I18nBehavior, PrefsBehavior],

  properties: {
    arcEnabledEnforced: Boolean,
    prefs: {type: Object, notify: true},
  },

  /**
   * @param {!Event} event
   * @private
   */
  onEnableTap_: function(event) {
    this.setPrefValue('arc.enabled', true);
    event.stopPropagation();
  },

  /** @return {boolean} */
  isEnforced_: function(pref) {
    return pref.enforcement == chrome.settingsPrivate.Enforcement.ENFORCED;
  },
});
