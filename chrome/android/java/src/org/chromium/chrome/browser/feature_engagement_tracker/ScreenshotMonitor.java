// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feature_engagement_tracker;

import android.os.Environment;
import android.os.FileObserver;
import android.os.StrictMode;

import org.chromium.base.ThreadUtils;

import java.io.File;

/**
 * This class detects screenshots by monitoring the screenshots directory on the sdcard and notifies
 * the ScreenshotMonitorDelegate.
 */
public class ScreenshotMonitor {
    private final ScreenshotMonitorDelegate mDelegate;
    final ScreenshotMonitorFileObserver mFileObserver;
    /**
     * This tracks whether monitoring is on (i.e. started but not stopped). It is OK to
     * access this from the UI thread.
     */
    private boolean mIsMonitoring;

    /**
     * This class requires the caller (a ScreenshotMonitor) to call setScreenshotMonitor after
     * instantiation the class.
     */
    static class ScreenshotMonitorFileObserver extends FileObserver {
        // TODO(angelashao): Generate screenshot directory path based on device.
        static String getDirPath() {
            String path;
            StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
            try {
                path = Environment.getExternalStorageDirectory().getPath();
            } finally {
                StrictMode.setThreadPolicy(oldPolicy);
            }
            return path + File.separator + Environment.DIRECTORY_PICTURES + File.separator
                    + "Screenshots";
        }

        private ScreenshotMonitor mScreenshotMonitor;

        public ScreenshotMonitorFileObserver() {
            super(getDirPath(), FileObserver.CREATE);
        }

        public void setScreenshotMonitor(ScreenshotMonitor monitor) {
            ThreadUtils.assertOnUiThread();
            mScreenshotMonitor = monitor;
        }

        @Override
        public void onEvent(final int event, final String path) {
            ThreadUtils.postOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if (mScreenshotMonitor == null) return;
                    mScreenshotMonitor.onEventOnUiThread(event, path);
                }
            });
        }
    }

    ScreenshotMonitor(
            ScreenshotMonitorDelegate delegate, ScreenshotMonitorFileObserver fileObserver) {
        mDelegate = delegate;
        mFileObserver = fileObserver;
        mFileObserver.setScreenshotMonitor(this);
    }

    public static ScreenshotMonitor create(ScreenshotMonitorDelegate delegate) {
        return new ScreenshotMonitor(delegate, new ScreenshotMonitorFileObserver());
    }

    /**
     * Start monitoring the screenshot directory.
     */
    public void startMonitoring() {
        ThreadUtils.assertOnUiThread();
        mFileObserver.startWatching();
        mIsMonitoring = true;
    }

    /**
     * Stop monitoring the screenshot directory.
     */
    public void stopMonitoring() {
        ThreadUtils.assertOnUiThread();
        mFileObserver.stopWatching();
        mIsMonitoring = false;
    }

    private void onEventOnUiThread(final int event, final String path) {
        if (!mIsMonitoring) return;
        if (path == null) return;
        assert event == FileObserver.CREATE;

        mDelegate.onScreenshotTaken();
    }
}
