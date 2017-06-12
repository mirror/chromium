// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feature_engagement_tracker;

import android.os.Environment;
import android.os.FileObserver;

import junit.framework.Assert;

import org.chromium.base.ThreadUtils;

import java.io.File;

/**
 * This class detects screenshots by monitoring the screenshots directory on the sdcard.
 */
public class ScreenshotMonitor extends FileObserver {
    protected ScreenshotMonitorDelegate mDelegate;
    private boolean mIsMonitoring;

    private static String sDIRPATH = Environment.getExternalStorageDirectory().getPath()
            + File.separator + Environment.DIRECTORY_PICTURES + File.separator + "Screenshots";

    public ScreenshotMonitor(ScreenshotMonitorDelegate delegate) {
        super(sDIRPATH, FileObserver.CREATE);
        mDelegate = delegate;
        mIsMonitoring = false;
    }

    public void startMonitoring() {
        Assert.assertTrue(ThreadUtils.runningOnUiThread());
        startWatching();
        mIsMonitoring = true;
    }

    public void stopMonitoring() {
        Assert.assertTrue(ThreadUtils.runningOnUiThread());
        stopWatching();
        mIsMonitoring = false;
    }

    public void onEvent(int event, String path) {
        if (!mIsMonitoring) return;
        if (path == null) return;
        Assert.assertEquals(FileObserver.CREATE, event);

        postOnUi();
    }

    protected void postOnUi() {
        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                mDelegate.onScreenshotTaken();
            }
        });
    }
}