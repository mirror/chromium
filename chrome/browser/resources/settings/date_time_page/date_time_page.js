// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-date-time-page' is the settings page containing date and time
 * settings.
 */

cr.exportPath('settings');

/**
 * Describes the effective policy restriction on time zone automatic detection.
 * @enum {number}
 */
settings.TimeZoneAutoDetectPolicyRestriction = {
  NONE: 0,
  FORCED_ON: 1,
  FORCED_OFF: 2,
};

/**
 * Describes values of prefs.settings.resolve_timezone_by_geolocation_method .
 * Must be kept in sync with TimeZoneResolverManager::TimeZoneResolveMethod
 * enum.
 * @enum {number}
 */
settings.TimeZoneAutoDetectMethod = {
  DISABLED: 0,
  IP_ONLY: 1,
  SEND_WIFI_ACCESS_POINTS: 2,
  SEND_ALL_LOCATION_INFO: 3
};

// SystemTimezoneProto_AutomaticTimezoneDetectionType_USERS_DECIDE = 0,
// SystemTimezoneProto_AutomaticTimezoneDetectionType_DISABLED = 1,
// SystemTimezoneProto_AutomaticTimezoneDetectionType_IP_ONLY = 2,
// SystemTimezoneProto_AutomaticTimezoneDetectionType_SEND_WIFI_ACCESS_POINTS = 3,
// SystemTimezoneProto_AutomaticTimezoneDetectionType_SEND_ALL_LOCATION_INFO = 4

Polymer({
  is: 'settings-date-time-page',

  behaviors: [
    PrefsBehavior,
    WebUIListenerBehavior
  ],

  properties: {
    /**
     * The effective policy restriction on time zone automatic detection.
     * @private {settings.TimeZoneAutoDetectPolicyRestriction}
     */
    timeZoneAutoDetectPolicyRestriction_: {
      type: Number,
      value: function() {
        if (!loadTimeData.valueExists('timeZoneAutoDetectValueFromPolicy'))
          return settings.TimeZoneAutoDetectPolicyRestriction.NONE;
        return loadTimeData.getBoolean('timeZoneAutoDetectValueFromPolicy') ?
            settings.TimeZoneAutoDetectPolicyRestriction.FORCED_ON :
            settings.TimeZoneAutoDetectPolicyRestriction.FORCED_OFF;
      },
    },

    /**
     * Whether a policy controls the time zone auto-detect setting.
     * @private
     */
    hasTimeZoneAutoDetectPolicyRestriction_: {
      type: Boolean,
      computed:
          'computeHasTimeZoneAutoDetectPolicy_(timeZoneAutoDetectPolicyRestriction_)',
    },

    /**
     * The effective time zone auto-detect enabled/disabled status.
     * @private
     */
    timeZoneAutoDetect_: {
      type: Boolean,
      computed: 'computeTimeZoneAutoDetect_(' +
          'timeZoneAutoDetectPolicyRestriction_,' +
          'prefs.settings.resolve_timezone_by_geolocation_method.value)',
    },

    /**
     * The effective time zone auto-detect method.
     * @private {settings.TimeZoneAutoDetectMethod}
     */
    timeZoneAutoDetectMethod_: {
      type: Number,
      computed: 'computeTimeZoneAutoDetectMethod_(' +
          'hasTimeZoneAutoDetectPolicyRestriction_,' +
          'prefs.settings.resolve_timezone_by_geolocation_method.value)',
    },

    /**
     * Whether date and time are settable. Normally the date and time are forced
     * by network time, so default to false to initially hide the button.
     * @private
     */
    canSetDateTime_: {
      type: Boolean,
      value: false,
    },
    /** @private {!Map<string, string>} */
    focusConfig_: {
      type: Object,
      value: function() {
        var map = new Map();
        if (settings.routes.DATETIME_TIMEZONE_SUBPAGE)
          map.set(settings.routes.DATETIME_TIMEZONE_SUBPAGE.path, '#time-zone-settings-subpage-trigger.subpage-arrow');
        return map;
      },
    },
  },

  /** @override */
  attached: function() {
    this.addWebUIListener(
        'time-zone-auto-detect-policy',
        this.onTimeZoneAutoDetectPolicyChanged_.bind(this));
    this.addWebUIListener(
        'can-set-date-time-changed', this.onCanSetDateTimeChanged_.bind(this));

    chrome.send('dateTimePageReady');
  },

  /**
   * @param {boolean} managed Whether the auto-detect setting is controlled.
   * @param {boolean} valueFromPolicy The value of the auto-detect setting
   *     forced by policy.
   * @private
   */
  onTimeZoneAutoDetectPolicyChanged_: function(managed, valueFromPolicy) {
    if (managed) {
      this.timeZoneAutoDetectPolicyRestriction_ = valueFromPolicy ?
          settings.TimeZoneAutoDetectPolicyRestriction.FORCED_ON :
          settings.TimeZoneAutoDetectPolicyRestriction.FORCED_OFF;
    } else {
      this.timeZoneAutoDetectPolicyRestriction_ = settings.TimeZoneAutoDetectPolicyRestriction.NONE;
    }
  },

  /**
   * @param {boolean} canSetDateTime Whether date and time are settable.
   * @private
   */
  onCanSetDateTimeChanged_: function(canSetDateTime) {
    this.canSetDateTime_ = canSetDateTime;
  },

  /**
   * @param {!Event} e
   * @private
   */
  onTimeZoneAutoDetectChange_: function(e) {
    this.setPrefValue(
        'settings.resolve_timezone_by_geolocation_method',
        e.target.checked ? settings.TimeZoneAutoDetectMethod.IP_ONLY :
                           settings.TimeZoneAutoDetectMethod.DISABLED);
  },

  /** @private */
  onSetDateTimeTap_: function() {
    chrome.send('showSetDateTimeUI');
  },

  /**
   * @param {settings.TimeZoneAutoDetectPolicyRestriction} timeZoneAutoDetectPolicy
   * @return {boolean}
   * @private
   */
  computeHasTimeZoneAutoDetectPolicy_: function(timeZoneAutoDetectPolicy) {
    return timeZoneAutoDetectPolicy != settings.TimeZoneAutoDetectPolicyRestriction.NONE;
  },

  /**
   * @param {settings.TimeZoneAutoDetectPolicyRestriction} timeZoneAutoDetectPolicy
   * @param {settings.TimeZoneAutoDetectMethod} prefValue
   *     prefs.settings.resolve_timezone_by_geolocation_method.value
   * @return {settings.TimeZoneAutoDetectPolicyRestriction} Whether time zone auto-detect is enabled.
   * @private
   */
  computeTimeZoneAutoDetect_: function(timeZoneAutoDetectPolicy, prefValue) {
    switch (timeZoneAutoDetectPolicy) {
      case settings.TimeZoneAutoDetectPolicyRestriction.NONE:
        return prefValue != settings.TimeZoneAutoDetectMethod.DISABLED;
      case settings.TimeZoneAutoDetectPolicyRestriction.FORCED_ON:
        return true;
      case settings.TimeZoneAutoDetectPolicyRestriction.FORCED_OFF:
        return false;
      default:
        assertNotReached();
    }
  },

  /**
   * Computes effective time zone detection method.
   * @param {Boolean} hasTimeZoneAutoDetectPolicyRestriction
   *     this.hasTimeZoneAutoDetectPolicyRestriction_
   * @param {settings.TimeZoneAutoDetectMethod} prefResolveValue
   *     prefs.settings.resolve_timezone_by_geolocation_method.value
   * @return {settings.TimeZoneAutoDetectMethod}
   * @private
   */
  computeTimeZoneAutoDetectMethod_: function(hasTimeZoneAutoDetectPolicyRestriction,
      prefResolveValue) {
    //console.error("TimeZoneAutoDetectMethod: hasTimeZoneAutoDetectPolicyRestriction=" + hasTimeZoneAutoDetectPolicyRestriction + ", prefResolveValue=" + prefResolveValue + "): settings.resolve_device_timezone_by_geolocation_policy=" + JSON.stringify(this.getPref('settings.resolve_device_timezone_by_geolocation_policy'), null, 2));
    if (hasTimeZoneAutoDetectPolicyRestriction) {
      // timeZoneAutoDetectPolicyRestriction_ actually depends on several time
      // policies and chrome flags. So we ignore real policy value if it is
      // disabled.
      if (this.timeZoneAutoDetectPolicyRestriction_ == settings.TimeZoneAutoDetectPolicyRestriction.FORCED_OFF)
        return settings.TimeZoneAutoDetectMethod.DISABLED;

      return this.getPref('settings.resolve_device_timezone_by_geolocation_policy').value;
    }
    return prefResolveValue;
  },

  /**
   * Returns true if given time zone resolve method is IP_ONLY.
   * @param {settings.TimeZoneAutoDetectMethod} method
   *     this.timeZoneAutoDetectMethod_ value.
   * @return {Boolean}
   * @private
   */
  isTimezoneDetectionMethodIpOnly_: function(method) {
    return method == settings.TimeZoneAutoDetectMethod.IP_ONLY;
  },

  /**
   * Returns true if given time zone resolve method is SEND_WIFI_ACCESS_POINTS.
   * @param {settings.TimeZoneAutoDetectMethod} method
   *     this.timeZoneAutoDetectMethod_ value.
   * @return {Boolean}
   * @private
   */
  isTimezoneDetectionMethodSendWiFiAccessPoints_: function(method) {
    return method == settings.TimeZoneAutoDetectMethod.SEND_WIFI_ACCESS_POINTS;
  },

  /**
   * Returns true if given time zone resolve method is SEND_ALL_LOCATION_INFO.
   * @param {settings.TimeZoneAutoDetectMethod} method
   *     this.timeZoneAutoDetectMethod_ value.
   * @return {Boolean}
   * @private
   */
  isTimezoneDetectionMethodSendAllLocationInfo_: function(method) {
    return method == settings.TimeZoneAutoDetectMethod.SEND_ALL_LOCATION_INFO;
  },

  onTimeZoneSettings_: function() {
    console.error("----------- Button pressed!----");
    settings.navigateTo(settings.routes.DATETIME_TIMEZONE_SUBPAGE);
  },
});
