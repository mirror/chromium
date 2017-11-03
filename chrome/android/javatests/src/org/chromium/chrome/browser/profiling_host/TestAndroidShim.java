// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.profiling_host;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;

// import android.annotation.SuppressLint;
// import android.content.BroadcastReceiver;
// import android.content.Context;
// import android.content.Intent;
// import android.content.IntentFilter;
// import android.os.Environment;

// import org.chromium.base.VisibleForTesting;

/**
 * Provides direct access to ProfilingProcessHost. Only used for testing.
 */
// @JNINamespace("chrome::android::profiling")
// @MainDex
// @VisibleForTesting
public class TestAndroidShim {
    private static final String TAG = "cr.ProfilingProcessHost";

    public TestAndroidShim() {
    }

    /**
     * Start profiling all allocations for the browser process.
     */
    public void startProfilingBrowserProcess() {
        Log.e(TAG, "startProfilingBrowserProcess1.");
        initializeNativeControllerIfNeeded();
        Log.e(TAG, "startProfilingBrowserProcess2.");
        nativeStartProfilingBrowserProcess(mNativeTestAndroidShim);
        Log.e(TAG, "startProfilingBrowserProcess3.");
    }

    /**
     * Starts tracing, waits for the heap dump to be added to the trace and then
     * calls onTraceFinished() with the serialized Trace as a string.
     */
    public void takeTrace() {
        initializeNativeControllerIfNeeded();
        nativeTakeTrace(mNativeTestAndroidShim);
    }

    @CalledByNative
    protected void onTraceFinished(String trace) {
        Log.e(TAG, "onTraceFinished.");
        Log.e(TAG, trace);
    }

    private void initializeNativeControllerIfNeeded() {
        if (mNativeTestAndroidShim == 0) {
            mNativeTestAndroidShim = nativeInit();
        }
    }

    /**
     * Clean up the C++ side of this class.
     * After the call, this class instance shouldn't be used.
     */
    public void destroy() {
        if (mNativeTestAndroidShim != 0) {
            nativeDestroy(mNativeTestAndroidShim);
            mNativeTestAndroidShim = 0;
        }
    }

    private long mNativeTestAndroidShim;
    private native long nativeInit();
    private native void nativeDestroy(long nativeTestAndroidShim);
    private native void nativeStartProfilingBrowserProcess(long nativeTestAndroidShim);
    private native void nativeTakeTrace(long nativeTestAndroidShim);
}
