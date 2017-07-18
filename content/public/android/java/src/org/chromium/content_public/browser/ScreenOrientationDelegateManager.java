// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import android.app.Activity;

/**
 * An static class used for managing registration of ScreenOrientationDelegates.
 */
public class ScreenOrientationDelegateManager {
    private static ScreenOrientationDelegate sDelegate;

    /**
     * Notify the delegate that ScreenOrientationProvider consumers would like to unlock orientation
     * for an activity. Returns true if ScreenOrientationProvider should unlock orientation, and
     * false if the delegate already handled it.
     */
    public static boolean canUnlockOrientation(Activity activity, int defaultOrientation) {
        return sDelegate != null ? sDelegate.canUnlockOrientation(activity, defaultOrientation)
                                 : true;
    }

    /**
     * Allows the delegate to control whether ScreenOrientationProvider clients
     * can lock orientation.
     */
    public static boolean canLockOrientation() {
        return sDelegate != null ? sDelegate.canLockOrientation() : true;
    }

    /**
     * Sets the current ScreenOrientationDelegate.
     */
    public static void setOrientationDelegate(ScreenOrientationDelegate delegate) {
        sDelegate = delegate;
    }
}