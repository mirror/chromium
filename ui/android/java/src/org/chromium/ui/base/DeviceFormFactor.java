// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.base;

import android.content.Context;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.ui.display.DisplayAndroid;

/**
 * UI utilities for accessing form factor information.          s
 */
public class DeviceFormFactor {
    // TODO(agrieve): Make this private to force callsites to consider passing a context (once
    //     downstream code is updated).
    /**
     * @return Whether the current primary display of the app is large enough to be considered a
     *         tablet. Not affected by Android N multi-window, but can change for external displays.
     *         E.g. http://developer.samsung.com/samsung-dex/testing
     */
    @CalledByNative
    public static boolean isTablet() {
        return isTablet(ContextUtils.getApplicationContext());
    }

    /**
     * @return Whether the display associated with the given context is large enough to be
     *         considered a tablet.
     */
    public static boolean isTablet(Context context) {
        return DisplayAndroid.getNonMultiDisplay(context).isTablet();
    }
}