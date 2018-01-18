// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import android.annotation.SuppressLint;
import android.os.Build;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Java implementation of CastSysInfoAndroid methods.
 */
@JNINamespace("chromecast")
public final class CastSysInfoAndroid {
    private static final String TAG = "CastSysInfoAndroid";

    @SuppressLint("HardwareIds")
    @SuppressWarnings("deprecation")
    @CalledByNative
    private static String getSerialNumber() {
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.N_MR1) {
            try {
                return Build.getSerial();
            } catch (SecurityException e) {
                Log.e(TAG, "Cannot get device serial number.");
                return "";
            }
        }
        return Build.SERIAL;
    }

    @SuppressLint("HardwareIds")
    @CalledByNative
    private static String getBoard() {
        return Build.BOARD;
    }
}
