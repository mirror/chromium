// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.test.util;

import android.content.Context;
import android.text.TextUtils;

import org.chromium.base.test.util.DisableIfSkipCheck;
import org.chromium.ui.display.DisplayAndroid;

/**
 * Checks for conditional disables. Currently only includes checks against
 * a few device form factor values.
 */
public class UiDisableIfSkipCheck extends DisableIfSkipCheck {
    private final Context mTargetContext;

    public UiDisableIfSkipCheck(Context targetContext) {
        mTargetContext = targetContext;
    }

    @Override
    protected boolean deviceTypeApplies(String type) {
        DisplayAndroid display = DisplayAndroid.getNonMultiDisplay(mTargetContext);
        if (TextUtils.equals(type, UiDisableIf.PHONE) && !display.isTablet()) {
            return true;
        }
        if (TextUtils.equals(type, UiDisableIf.TABLET) && display.isTablet()) {
            return true;
        }
        if (TextUtils.equals(type, UiDisableIf.LARGETABLET) && display.isLargeTablet()) {
            return true;
        }
        return false;
    }
}
