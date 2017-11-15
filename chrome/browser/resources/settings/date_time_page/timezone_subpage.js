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
     * The effective time zone auto-detect enabled/disabled status.
     * @private
     */
    timeZoneAutoDetect: Boolean,

    /**
     * settings.TimeZoneAutoDetectMethod values.
     * @private {!Object<settings.TimeZoneAutoDetectMethod, number>}
     */
    timezoneAutodetectMethodValues_: Object,

    /**
     * This is <timezone-selector> parameter.
     */
    activeTimeZoneDisplayName: {
      type: String,
      notify: true,
    },
  },

  attached: function() {
    this.timezoneAutodetectMethodValues_ = settings.TimeZoneAutoDetectMethod;
  },
});
})();
