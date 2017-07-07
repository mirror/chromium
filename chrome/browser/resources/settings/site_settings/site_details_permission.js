// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'site-details-permission' handles showing the state of one permission, such
 * as Geolocation, for a given origin.
 */
Polymer({
  is: 'site-details-permission',

  behaviors: [SiteSettingsBehavior, WebUIListenerBehavior],

  properties: {
    /**
     * The site that this widget is showing details for.
     * @type {SiteException}
     */
    site: Object,

    /**
     * The default setting string for this permission category.
     * @type {string}
     * @private
     */
    defaultSettingString_: {
      type: String,
    }
  },

  observers: ['siteChanged_(site, category)'],

  /** @override */
  attached: function() {
    this.addWebUIListener(
        'contentSettingSitePermissionChanged',
        this.sitePermissionChanged_.bind(this));
  },

  /**
   * Returns true if the origins match, e.g. http://google.com and
   * http://[*.]google.com.
   * @param {string} left The first origin to compare.
   * @param {string} right The second origin to compare.
   * @return {boolean} True if the origins are the same.
   * @private
   */
  sameOrigin_: function(left, right) {
    return this.removePatternWildcard(left) ==
        this.removePatternWildcard(right);
  },

  /**
   * Updates the drop-down value after |site| has changed.
   * @param {!SiteException} site The site to display.
   * @private
   */
  siteChanged_: function(site) {
    if (site.enforcement == chrome.settingsPrivate.Enforcement.RECOMMENDED) {
      this.updateDefaultSettingString_(site.setting);
      this.$.permission.value = settings.ContentSetting.DEFAULT;
      return;
    }
    this.updateDefaultSettingString_(null);
    this.$.permission.value = site.setting;
  },

  /**
   * Called when a site within a category has been changed.
   * @param {number} category The category that changed.
   * @param {string} origin The origin of the site that changed.
   * @param {string} embeddingOrigin The embedding origin of the site that
   *     changed.
   * @private
   */
  sitePermissionChanged_: function(category, origin, embeddingOrigin) {
    if (this.site === undefined)
      return;
    if (category != this.category)
      return;

    if (origin == '' ||
        (origin == this.site.origin &&
         embeddingOrigin == this.site.embeddingOrigin)) {
      this.siteChanged_(this.site);
    }
  },

  /**
   * Resets the category permission for this origin.
   */
  resetPermission: function() {
    this.browserProxy.resetCategoryPermissionForOrigin(
        this.site.origin, this.site.embeddingOrigin, this.category,
        this.site.incognito);
  },

  /**
   * Handles the category permission changing for this origin.
   * @private
   */
  onPermissionSelectionChange_: function() {
    if (this.$.permission.value == settings.ContentSetting.DEFAULT) {
      this.resetPermission();
      return;
    }
    this.browserProxy.setCategoryPermissionForOrigin(
        this.site.origin, this.site.embeddingOrigin, this.category,
        this.$.permission.value, this.site.incognito);
  },

  /**
   * Updates the string used for this permission category's default setting.
   * @param {?settings.ContentSetting} defaultSetting The default setting, if
   *    known.
   * @private
   */
  updateDefaultSettingString_: function(defaultSetting) {
    var getDefaultSettingString = function(setting) {
      var stringId;
      if (setting == settings.ContentSetting.ASK ||
          setting == settings.ContentSetting.IMPORTANT_CONTENT)
        stringId = 'siteSettingsActionAskDefault';
      else if (setting == settings.ContentSetting.ALLOW)
        stringId = 'siteSettingsActionAllowDefault';
      else if (setting == settings.ContentSetting.BLOCK)
        stringId = 'siteSettingsActionBlockDefault';
      assert(
          stringId,
          'No string for ' + this.category + '\'s default setting of ' +
              setting);
      this.defaultSettingString_ = loadTimeData.getString(stringId);
    }.bind(this);

    if (defaultSetting != null) {
      getDefaultSettingString(defaultSetting);
      return;
    }

    // The default setting is unknown, so consult the C++ backend for it.
    this.browserProxy.getDefaultValueForContentType(this.category)
        .then(function(defaultValue) {
          getDefaultSettingString(defaultValue.setting);
        });
  },
});
