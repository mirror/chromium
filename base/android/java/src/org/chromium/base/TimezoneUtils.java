// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import org.chromium.base.annotations.CalledByNative;

import java.util.TimeZone;

class TimezoneUtils {
    /**
     * Guards this class from being instantiated.
     */

    private TimezoneUtils() {}

    /**
     * @return the Olson timezone ID of the current system time zone.
     */
    @CalledByNative
    public static String getDefaultTimeZoneId() {
        return TimeZone.getDefault().getID();
    }
}
