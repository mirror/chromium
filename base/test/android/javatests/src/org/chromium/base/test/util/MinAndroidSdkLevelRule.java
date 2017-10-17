// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.util;

import android.os.Build;

import org.junit.runner.Description;

import org.chromium.base.Log;

public class MinAndroidSdkLevelRule extends SkipRule {
    private static final String TAG = "MinSdkRule";

    @Override
    public boolean shouldSkip(Description desc) {
        int minSdkLevel = 0;
        MinAndroidSdkLevel annotation = desc.getAnnotation(MinAndroidSdkLevel.class);
        if (annotation != null) {
            minSdkLevel = Math.max(minSdkLevel, annotation.value());
        }
        if (Build.VERSION.SDK_INT < minSdkLevel) {
            Log.i(TAG, "Test " + desc.getClassName() + "#" + desc.getMethodName()
                            + " is not enabled at SDK level " + Build.VERSION.SDK_INT + ".");
            return true;
        }
        return false;
    }
}
