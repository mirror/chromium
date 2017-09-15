// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.crash;

import org.chromium.base.annotations.MainDex;

/**
 * This UncaughtExceptionHandler will upload the stacktrace when there is an uncaught exception.
 *
 * This happens before native is loaded, and will replace by JavaExceptionReporter after native
 * finishes loading.
 */
@MainDex
public class PureJavaExceptionHandler implements Thread.UncaughtExceptionHandler {
    private final Thread.UncaughtExceptionHandler mParent;
    private boolean mHandlingException;
    private static boolean sIsDisabled = false;
    private static PureJavaExceptionHandler sInstance = null;

    private PureJavaExceptionHandler(Thread.UncaughtExceptionHandler parent) {
        mParent = parent;
    }

    @Override
    public void uncaughtException(Thread t, Throwable e) {
        if (!mHandlingException && !sIsDisabled) {
            mHandlingException = true;
            reportJavaException(e);
        }
        if (mParent != null) {
            mParent.uncaughtException(t, e);
        }
    }

    public static void installHandler() {
        if (sInstance == null) {
            sInstance = new PureJavaExceptionHandler(Thread.getDefaultUncaughtExceptionHandler());
        }
        Thread.setDefaultUncaughtExceptionHandler(sInstance);
    }

    public static void uninstallHandler() {
        sIsDisabled = true;
    }

    private void reportJavaException(Throwable e) {
        PureJavaExceptionReporter exceptionReporter = new PureJavaExceptionReporter();
        exceptionReporter.createAndUploadReport(e);
    }
}
