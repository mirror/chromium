// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('print_preview_new');
/**
 * @typedef {{
 *   value: *,
 *   valid: boolean,
 *   available: boolean,
 *   key: string,
 * }}
 */
print_preview_new.Setting;

/**
 * @typedef {{
 *    version: string,
 *    recentDestinations: (!Array<!print_preview.RecentDestination> |
 *                         undefined),
 *    dpi: ({horizontal_dpi: number,
 *           vertical_dpi: number,
 *           is_default: (boolean | undefined)} | undefined),
 *    mediaSize: ({height_microns: number,
 *                 width_microns: number,
 *                 custom_display_name: (string | undefined),
 *                 is_default: (boolean | undefined)} | undefined),
 *    marginsType: (print_preview.ticket_items.MarginsTypeValue | undefined),
 *    customMargins: ({marginTop: number,
 *                     marginBottom: number,
 *                     marginLeft: number,
 *                     marginRight: number} | undefined),
 *    isColorEnabled: (boolean | undefined),
 *    isDuplexEnabled: (boolean | undefined),
 *    isHeaderFooterEnabled: (boolean | undefined),
 *    isLandscapeEnabled: (boolean | undefined),
 *    isCollateEnabled: (boolean | undefined),
 *    isFitToPageEnabled: (boolean | undefined),
 *    isCssBackgroundEnabled: (boolean | undefined),
 *    scaling: (string | undefined),
 *    vendor_options: (Object | undefined)
 * }}
 */
print_preview_new.SerializedSettings;

/**
 * @typedef {{
 *   pages: !print_preview_new.Setting,
 *   copies: !print_preview_new.Setting,
 *   collate: !print_preview_new.Setting,
 *   layout: !print_preview_new.Setting,
 *   color: !print_preview_new.Setting,
 *   mediaSize: !print_preview_new.Setting,
 *   margins: !print_preview_new.Setting,
 *   dpi: !print_preview_new.Setting,
 *   fitToPage: !print_preview_new.Setting,
 *   scaling: !print_preview_new.Setting,
 *   duplex: !print_preview_new.Setting,
 *   cssBackground: !print_preview_new.Setting,
 *   selectionOnly: !print_preview_new.Setting,
 *   headerFooter: !print_preview_new.Setting,
 *   rasterize: !print_preview_new.Setting,
 *   vendorItems: !print_preview_new.Setting,
 *   otherOptions: !print_preview_new.Setting,
 *   serialization: !print_preview_new.SerializedSettings,
 *   initialized: boolean,
 * }}
 */
print_preview_new.Settings;

/** @polymerBehavior */
const SettingsBehavior = {
  properties: {
    /** @type {print_preview_new.Settings} */
    settings: {
      type: Object,
      notify: true,
    },
  },

  /**
   * @param {string} settingName Name of the setting to get.
   * @return {print_preview_new.Setting} The setting object.
   */
  getSetting: function(settingName) {
    const setting = /** @type {print_preview_new.Setting} */ (
        this.get(settingName, this.settings));
    assert(!!setting, 'Setting is missing: ' + settingName);
    return setting;
  },

  /**
   * @param {string} settingName Name of the setting to set
   * @param {boolean | string | number | Array | Object} value The value to set
   *     the setting to.
   */
  setSetting: function(settingName, value) {
    const setting = this.getSetting(settingName);
    this.set(`settings.${settingName}.value`, value);
    this.persistSetting(setting);
  },

  /**
   * @param {!print_preview_new.Setting} setting The setting object that was
   *     set.
   * @private
   */
  persistSetting: function(setting) {
    if (setting.valid && setting.key.length > 0 && this.settings.initialized) {
      this.set(`settings.serialization.${setting.key}`, setting.value);
      const nativeLayer = print_preview.NativeLayer.getInstance();
      nativeLayer.saveAppState(JSON.stringify(this.settings.serialization));
    }
  },

  /**
   * @param {string} settingName Name of the setting to set
   * @param {boolean} valid Whether the setting value is currently valid.
   */
  setSettingValid: function(settingName, valid) {
    const setting = this.getSetting(settingName);
    // Should not set the setting to invalid if it is not available, as there
    // is no way for the user to change the value in this case.
    if (!valid)
      assert(setting.available, 'Setting is not available: ' + settingName);
    this.set(`settings.${settingName}.valid`, valid);
  }
};
