// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'timezone-subpage' is the collapsible section containing
 * time zone settings.
 */


(function() {
'use strict';

Polymer({
  is: 'timezone-subpage',

  behaviors: [PrefsBehavior],

  properties: {
    /**
     * The effective policy restriction on time zone automatic detection.
     * @private {settings.TimeZoneAutoDetectPolicyRestriction}
     */
    timeZoneAutoDetectPolicyRestriction: Number,

    /**
     * Whether a policy controls the time zone auto-detect setting.
     * @private
     */
    hasTimeZoneAutoDetectPolicyRestriction: Boolean,

    /**
     * The effective time zone auto-detect enabled/disabled status.
     * @private
     */
    timeZoneAutoDetect: Boolean,

    /**
     * The effective time zone auto-detect method.
     * @private {settings.TimeZoneAutoDetectMethod}
     */
    timeZoneAutoDetectMethod: Number,

    /**
     * settings.TimeZoneAutoDetectMethod values.
     * @private
     */
    timezoneAutodetectMethodValues_: {
      type: Object,
      value: settings.TimeZoneAutoDetectMethod,
      readOnly: true,
    },
  },

});
})();
