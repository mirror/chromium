// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.locale;

import android.support.v4.text.TextUtilsCompat;
import android.support.v4.view.ViewCompat;

import org.chromium.base.LocaleUtils;

import java.util.Locale;

/**
 * Locale utilities specific to the Chrome browser.
 */
public class ChromeLocaleUtils {
    private ChromeLocaleUtils() {}

    /**
     * @return The locale of the Chrome application.  This represents the locale of the resources
     *         loaded in Chrome and can differ from {@link Locale#getDefault()} in cases where
     *         Chrome does not support the current Android locale.
     */
    public static Locale getApplicationLocale() {
        String localeString = nativeGetApplicationLocaleForJava();
        return LocaleUtils.forLanguageTagCompat(localeString);
    }

    /**
     * @return Whether the application locale is RTL.
     */
    public static boolean isApplicationLocaleRtl() {
        return TextUtilsCompat.getLayoutDirectionFromLocale(getApplicationLocale())
                == ViewCompat.LAYOUT_DIRECTION_RTL;
    }

    private static native String nativeGetApplicationLocaleForJava();
}
