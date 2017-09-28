// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'category-setting-exceptions' is the polymer element for showing a certain
 * category of exceptions under Site Settings.
 */
Polymer({
  is: 'category-setting-exceptions',

  behaviors: [
    I18nBehavior,
    SiteSettingsBehavior,
  ],

  properties: {
    /**
     * Some content types (like Location) do not allow the user to manually
     * edit the exception list from within Settings.
     * @private
     */
    readOnlyList: {
      type: Boolean,
      value: false,
    },
  },

  /** @override */
  ready: function() {
    this.ContentSetting = settings.ContentSetting;

    // The "Block" list shows as "Muted" for the Sound setting.
    if (this.category == settings.ContentSettingsTypes.SOUND) {
      this.BlockHeader = this.i18n('siteSettingsBlockSound');
    } else {
      this.BlockHeader = this.i18n('siteSettingsBlock');
    }
  },
});
