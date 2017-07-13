// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;
import android.app.Activity;

/**
 * An interface for ScreenOrientationProvider to notify other components that orientation
 * preferences may change.
 */
public interface ScreenOrientationDelegate {
    /**
     * Notify the delegate that ScreenOrientationProvider consumers would like to unlock orientation
     * for an activity. Returns true if ScreenOrientationProvider can unlock orientation.
     */
    boolean orientationUnlocked(Activity activity, int defaultOrientation);
}