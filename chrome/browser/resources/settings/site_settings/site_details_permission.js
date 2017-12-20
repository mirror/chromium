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

  behaviors: [SiteSettingsBehavior, WebUIListenerBehavior, I18nBehavior],

  properties: {
    /**
     * Constants storing the string IDs for the permission info strings
     * displayed underneath each permission.
     * @type {string}
     */
    infoStringEmptyId_: {
      type: String,
      readonly: true,
      value: 'siteSettingsSourceNormal',
    },
    /** @type {string} */
    infoStringAdsBlacklistId_: {
      type: String,
      readonly: true,
      value: 'siteSettingsSourceAdsBlacklist',
    },
    /** @type {string} */
    infoStringAdsBlockId_: {
      type: String,
      readonly: true,
      value: 'siteSettingsAdsBlockSingular',
    },
    /** @type {string} */
    infoStringDrmDisabledId_: {
      type: String,
      readonly: true,
      value: 'siteSettingsSourceDrmDisabled',
    },
    /** @type {string} */
    infoStringEmbargoId_: {
      type: String,
      readonly: true,
      value: 'siteSettingsSourceEmbargo',
    },
    /** @type {string} */
    infoStringInsecureOriginId_: {
      type: String,
      readonly: true,
      value: 'siteSettingsSourceInsecureOrigin',
    },
    /** @type {string} */
    infoStringKillSwitchId_: {
      type: String,
      readonly: true,
      value: 'siteSettingsSourceKillSwitch',
    },

    /**
     * The extension and policy strings differ depending on their current
     * content setting, so store them in object maps.
     * @type {Object<!settings.ContentSetting, string>} */
    infoStringExtensionIds_: {
      type: Object,
      readonly: true,
      value: {
        [[settings.ContentSetting.ALLOW]]: 'siteSettingsSourceExtensionAllow',
        [[settings.ContentSetting.BLOCK]]: 'siteSettingsSourceExtensionBlock',
        [[settings.ContentSetting.ASK]]: 'siteSettingsSourceExtensionAsk',
      },
    },
    /** @type {Object<!settings.ContentSetting, string>} */
    infoStringPolicyIds_: {
      type: Object,
      readonly: true,
      value: {
        [[settings.ContentSetting.ALLOW]]: 'siteSettingsSourcePolicyAllow',
        [[settings.ContentSetting.BLOCK]]: 'siteSettingsSourcePolicyBlock',
        [[settings.ContentSetting.ASK]]: 'siteSettingsSourcePolicyAsk',
      },
    },

    /**
     * The site that this widget is showing details for.
     * @type {RawSiteException}
     */
    site: Object,

    /**
     * The default setting for this permission category.
     * @type {settings.ContentSetting}
     * @private
     */
    defaultSetting_: String,

    /**
     * The permission information strings to be shown beneath each permission.
     * @type {?Map<string, string>}
     * @private
     */
    infoStringMap_: {
      type: Object,
      value: null,
      notify: true,
    }
  },

  observers: ['siteChanged_(site)'],

  /** @override */
  ready: function() {
    this.infoStringMap_ = /** @type Map<string, string> */ (JSON.parse(
        this.$.permissionInfoString.dataset['permissionInfoStrings']));
  },

  /** @override */
  attached: function() {
    this.addWebUIListener(
        'contentSettingCategoryChanged',
        this.onDefaultSettingChanged_.bind(this));
  },

  /**
   * Updates the drop-down value after |site| has changed.
   * @param {!RawSiteException} site The site to display.
   * @private
   */
  siteChanged_: function(site) {
    if (site.source == settings.SiteSettingSource.DEFAULT) {
      this.defaultSetting_ = site.setting;
      this.$.permission.value = settings.ContentSetting.DEFAULT;
    } else {
      // The default setting is unknown, so consult the C++ backend for it.
      this.updateDefaultPermission_(site);
      this.$.permission.value = site.setting;
    }

    if (this.isNonDefaultAsk_(site.setting, site.source)) {
      assert(
          this.$.permission.disabled,
          'The \'Ask\' entry is for display-only and cannot be set by the ' +
              'user.');
      assert(
          this.$.permission.value == settings.ContentSetting.ASK,
          '\'Ask\' should only show up when it\'s currently selected.');
    }
  },

  /**
   * Updates the default permission setting for this permission category.
   * @param {!RawSiteException} site The site to display.
   * @private
   */
  updateDefaultPermission_: function(site) {
    this.browserProxy.getDefaultValueForContentType(this.category)
        .then((defaultValue) => {
          this.defaultSetting_ = defaultValue.setting;
        });
  },

  /**
   * Handles the category permission changing for this origin.
   * @param {!settings.ContentSettingsTypes} category The permission category
   *     that has changed default permission.
   * @private
   */
  onDefaultSettingChanged_: function(category) {
    if (category == this.category)
      this.updateDefaultPermission_(this.site);
  },

  /**
   * Handles the category permission changing for this origin.
   * @private
   */
  onPermissionSelectionChange_: function() {
    this.browserProxy.setOriginPermissions(
        this.site.origin, [this.category], this.$.permission.value);
  },

  /**
   * Updates the string used for this permission category's default setting.
   * @param {!settings.ContentSetting} defaultSetting Value of the default
   *     setting for this permission category.
   * @param {string} askString 'Ask' label, e.g. 'Ask (default)'.
   * @param {string} allowString 'Allow' label, e.g. 'Allow (default)'.
   * @param {string} blockString 'Block' label, e.g. 'Blocked (default)'.
   * @private
   */
  defaultSettingString_: function(
      defaultSetting, askString, allowString, blockString) {
    if (defaultSetting == settings.ContentSetting.ASK ||
        defaultSetting == settings.ContentSetting.IMPORTANT_CONTENT) {
      return askString;
    } else if (defaultSetting == settings.ContentSetting.ALLOW) {
      return allowString;
    } else if (defaultSetting == settings.ContentSetting.BLOCK) {
      return blockString;
    }
    assertNotReached(
        `No string for ${this.category}'s default of ${defaultSetting}`);
  },

  /**
   * Returns true if there's a string to display that provides more information
   * about this permission's setting. Currently, this only gets called when
   * |this.site| is updated.
   * @param {!settings.SiteSettingSource} source The source of the permission.
   * @param {!settings.ContentSettingsTypes} category The permission type.
   * @param {!settings.ContentSetting} setting The permission setting.
   * @return {boolean} Whether the permission will have a source string to
   *     display.
   * @private
   */
  hasPermissionInfoString_: function(source, category, setting) {
    return this.permissionInfoStringKey_(source, category, setting) !=
        this.infoStringEmptyId_;
  },

  /**
   * Checks if there's a additional information to display, and returns the
   * class name to apply to permissions if so.
   * @param {!settings.SiteSettingSource} source The source of the permission.
   * @param {!settings.ContentSettingsTypes} category The permission type.
   * @param {!settings.ContentSetting} setting The permission setting.
   * @return {string} CSS class applied when there is an additional description
   *     string.
   * @private
   */
  permissionInfoStringClass_: function(source, category, setting) {
    return this.hasPermissionInfoString_(source, category, setting) ?
        'two-line' :
        '';
  },

  /**
   * Returns true if this permission can be controlled by the user.
   * @param {!settings.SiteSettingSource} source The source of the permission.
   * @return {boolean}
   * @private
   */
  isPermissionUserControlled_: function(source) {
    return !(
        source == settings.SiteSettingSource.DRM_DISABLED ||
        source == settings.SiteSettingSource.POLICY ||
        source == settings.SiteSettingSource.EXTENSION ||
        source == settings.SiteSettingSource.KILL_SWITCH ||
        source == settings.SiteSettingSource.INSECURE_ORIGIN);
  },

  /**
   * Returns true if the permission is set to a non-default 'ask'. Currently,
   * this only gets called when |this.site| is updated.
   * @param {!settings.ContentSetting} setting The setting of the permission.
   * @param {!settings.SiteSettingSource} source The source of the permission.
   * @private
   */
  isNonDefaultAsk_: function(setting, source) {
    if (setting != settings.ContentSetting.ASK ||
        source == settings.SiteSettingSource.DEFAULT) {
      return false;
    }

    assert(
        source == settings.SiteSettingSource.EXTENSION ||
            source == settings.SiteSettingSource.POLICY,
        'Only extensions or enterprise policy can change the setting to ASK.');
    return true;
  },

  /**
   * Returns the string ID to be used for this category's permission info
   * string.
   * @param {!settings.SiteSettingSource} source The source of the permission.
   * @param {!settings.ContentSettingsTypes} category The permission type.
   * @param {!settings.ContentSetting} setting The permission setting.
   * @return {string} The ID of the permission information string to display in
   *     the HTML.
   * @private
   */
  permissionInfoStringKey_: function(source, category, setting) {
    if (source == settings.SiteSettingSource.ADS_FILTER_BLACKLIST) {
      assert(
          settings.ContentSettingsTypes.ADS == category,
          'The ads filter blacklist only applies to Ads.');
      return this.infoStringAdsBlacklistId_;
    } else if (
        category == settings.ContentSettingsTypes.ADS &&
        setting == settings.ContentSetting.BLOCK) {
      return this.infoStringAdsBlockId_;
    } else if (source == settings.SiteSettingSource.DRM_DISABLED) {
      assert(
          settings.ContentSetting.BLOCK == setting,
          'If DRM is disabled, Protected Content must be blocked.');
      assert(
          settings.ContentSettingsTypes.PROTECTED_CONTENT == category,
          'The DRM disabled source only applies to Protected Content.');
      return this.infoStringDrmDisabledId_;
    } else if (source == settings.SiteSettingSource.EMBARGO) {
      assert(
          settings.ContentSetting.BLOCK == setting,
          'Embargo is only used to block permissions.');
      return this.infoStringEmbargoId_;
    } else if (source == settings.SiteSettingSource.EXTENSION) {
      return this.infoStringExtensionIds_[setting];
    } else if (source == settings.SiteSettingSource.INSECURE_ORIGIN) {
      assert(
          settings.ContentSetting.BLOCK == setting,
          'Permissions can only be blocked due to insecure origins.');
      return this.infoStringInsecureOriginId_;
    } else if (source == settings.SiteSettingSource.KILL_SWITCH) {
      assert(
          settings.ContentSetting.BLOCK == setting,
          'The permissions kill switch can only be used to block permissions.');
      return this.infoStringKillSwitchId_;
    } else if (source == settings.SiteSettingSource.POLICY) {
      return this.infoStringPolicyIds_[setting];
    } else if (
        source == settings.SiteSettingSource.DEFAULT ||
        source == settings.SiteSettingSource.PREFERENCE) {
      return this.infoStringEmptyId_;
    }
    assertNotReached(`No string for ${category} setting source '${source}'`);
    return this.infoStringEmptyId_;
  },

  /**
   * Updates the information string for the current permission.
   * Currently, this only gets called when |this.site| is updated.
   * @param {!settings.SiteSettingSource} source The source of the permission.
   * @param {!settings.ContentSettingsTypes} category The permission type.
   * @param {!settings.ContentSetting} setting The permission setting.
   * @param {?Map<string, string>} strings The array of strings used
   *     to give more information about permissions.
   * @return {?string} The permission information string to display in the HTML.
   *     Returns null if there are no strings to choose from (this could happen
   *     when the DOM has not been fully initialized yet).
   * @private
   */
  permissionInfoString_: function(source, category, setting, strings) {
    if (strings == null)
      return null;

    /** @type {string} */
    let infoStringId =
        this.permissionInfoStringKey_(source, category, setting, strings);

    if (infoStringId == this.infoStringDrmDisabledId_) {
      /** @type {I18nAdvancedOpts} */
      let options = {
        'substitutions':
            [settings.routes.SITE_SETTINGS_PROTECTED_CONTENT.getAbsolutePath()]
      };
      return this.i18nAdvanced(infoStringId, options);
    }
    return strings[infoStringId];
  },
});
