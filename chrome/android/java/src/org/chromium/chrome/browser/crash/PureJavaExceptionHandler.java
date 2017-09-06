// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.crash;

import android.util.Log;

import org.chromium.base.annotations.MainDex;
import org.chromium.base.annotations.SuppressFBWarnings;

/**
 * This UncaughtExceptionHandler will upload the stacktrace when there is an uncaught exception.
 *
 * This happens before native is loaded, and will replace by JavaExceptionReporter after native
 * finishes loading.
 */
@MainDex
public class PureJavaExceptionHandler implements Thread.UncaughtExceptionHandler {
    private final Thread.UncaughtExceptionHandler mParent;
    private final boolean mCrashAfterReport;
    private boolean mHandlingException;
    private static final String TAG = "PureJavaExceptionHandler";

    private PureJavaExceptionHandler(
            Thread.UncaughtExceptionHandler parent, boolean crashAfterReport) {
        mParent = parent;
        mCrashAfterReport = crashAfterReport;
    }

    @Override
    public void uncaughtException(Thread t, Throwable e) {
        if (!mHandlingException) {
            mHandlingException = true;
            reportJavaException(mCrashAfterReport, e);
        }
        if (mParent != null) {
            mParent.uncaughtException(t, e);
        }
    }

    public static void installHandler(boolean crashAfterReport) {
        Thread.setDefaultUncaughtExceptionHandler(new PureJavaExceptionHandler(
                Thread.getDefaultUncaughtExceptionHandler(), crashAfterReport));
    }

    @SuppressFBWarnings("DM_EXIT")
    private void reportJavaException(boolean crashAfterReport, Throwable e) {
        PureJavaExceptionReporter exceptionReporter = new PureJavaExceptionReporter();
        exceptionReporter.createAndUploadReport(e);
        if (crashAfterReport) {
            Log.e(TAG, "Uncaught exception" + Log.getStackTraceString(e));
            System.exit(-1);
        }
    }
}
