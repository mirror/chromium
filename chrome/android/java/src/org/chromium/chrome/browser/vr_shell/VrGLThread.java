// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.os.Looper;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Helper class for interfacing with the Android Choreographer from native code.
 */
@JNINamespace("vr_shell")
public class VrGLThread {
    @CalledByNative
    private static void prepareLooper() {
        Looper.prepare();
    }

    @CalledByNative
    private static void loopLooper() {
        Looper.loop();
    }

    @CalledByNative
    private static void quitLooper() {
        Looper.myLooper().quitSafely();
    }
}
