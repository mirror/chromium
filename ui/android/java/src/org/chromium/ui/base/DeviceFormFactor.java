// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.base;

import android.content.Context;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.ui.R;
import org.chromium.ui.display.DisplayAndroid;

/**
 * UI utilities for accessing form factor information.          s
 */
public class DeviceFormFactor {
    public static final int MINIMUM_TABLET_WIDTH_DP = 600;

    /**
     * @return Whether the current primary display of the app is large enough to be considered a
     *         tablet.
     */
    @CalledByNative
    public static boolean isTablet() {
        return isTabletDisplay(ContextUtils.getApplicationContext());
    }

    /**
     * @return Whether the current primary display of the app is large enough to be considered a
     *         tablet. Not affected by Android N multi-window, but can change for external displays.
     *         E.g. http://developer.samsung.com/samsung-dex/testing
     */
    public static boolean isTabletLayout(Context context) {
        boolean ret = isTabletDisplay(context);
        if (!ret) {
            // There have been no cases where tablet resources end up being used on phone-sized
            // displays. Short-circuit this common-case since checking resources is slower.
            return false;
        }
        // On some devices, OEM modifications to the resource loader cause phone resources to be
        // loaded even when a ResourceManager's configuration has smallestScreenWidthDp > 600.
        // Thus, the value here must be fetched from resources and not calculated.
        // See crbug.com/662338.
        return context.getResources().getInteger(R.integer.min_screen_width_bucket) > 1;
    }

    public static boolean isLargeTabletLayout(Context context) {
        return context.getResources().getInteger(R.integer.min_screen_width_bucket) > 2;
    }

    /**
     * @return The minimum width in px at which the display should be treated like a tablet for
     *         layout.
     */
    public static int getMinimumTabletWidthPx(Context context) {
        return getMinimumTabletWidthPx(
                DisplayAndroid.getNonMultiDisplay(ContextUtils.getApplicationContext()));
    }

    /**
     * @return The minimum width in px at which the display should be treated like a tablet for
     *         layout.
     */
    public static int getMinimumTabletWidthPx(DisplayAndroid display) {
        float dipScale = display.getDipScale();
        // Adding .5 is what Android does when converting from dp -> px.
        return (int) (DeviceFormFactor.MINIMUM_TABLET_WIDTH_DP * dipScale + 0.5f);
    }

    /**
     * @return Whether the display associated with the given context is at large enough to be
     *         considered a tablet.
     */
    private static boolean isTabletDisplay(Context context) {
        DisplayAndroid display = DisplayAndroid.getNonMultiDisplay(context);
        return display.getSmallestWidth() >= getMinimumTabletWidthPx(display);
    }
}
