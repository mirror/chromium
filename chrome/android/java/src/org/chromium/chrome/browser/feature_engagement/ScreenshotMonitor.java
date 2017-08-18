// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feature_engagement;

import android.database.Cursor;
import android.net.Uri;
import android.os.Environment;
import android.os.FileObserver;
import android.os.StrictMode;
import android.provider.MediaStore;
import android.util.Log;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;

import java.io.File;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

/**
 * This class detects screenshots by monitoring the screenshots directory on external and internal
 * storages and notifies the ScreenshotMonitorDelegate. The caller should use
 * @{link ScreenshotMonitor#create(ScreenshotMonitorDelegate)} to create an instance.
 */
public class ScreenshotMonitor {
    private static final String TAG = "ScreenshotMonitor";
    private final ScreenshotMonitorDelegate mDelegate;

    @VisibleForTesting
    final ArrayList<ScreenshotMonitorFileObserver> mFileObservers;
    /**
     * This tracks whether monitoring is on (i.e. started but not stopped). It must only be accessed
     * on the UI thread.
     */
    private boolean mIsMonitoring;

    private static File getStorageRoot() {
        // Check if external storage is available (non-removable) and readable
        String externalStorageState;
        File externalDir;
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        try {
            externalStorageState = Environment.getExternalStorageState();
            externalDir = Environment.getExternalStorageDirectory();
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }
        boolean isExternalReadable = Environment.MEDIA_MOUNTED.equals(externalStorageState)
                || Environment.MEDIA_MOUNTED_READ_ONLY.equals(externalStorageState)
                || !Environment.isExternalStorageRemovable();
        if (isExternalReadable) {
            return externalDir;
        }
        return null;
    }

    /**
     * Get all screenshot directories if external storage is available and readable.
     * @param file The file in which to search for screenshots directories.
     * @param screenshotDirs The list of screenshot directories found.
     */
    private static void getScreenshotDirs(File file, Set<File> screenshotDirs) {
        // Find all screenshot directories
        if (file.isDirectory()) {
            if (file.getName().startsWith("Screenshot")) {
                screenshotDirs.add(file);
            }
            for (File childFile : file.listFiles()) {
                getScreenshotDirs(childFile, screenshotDirs);
            }
        }

        Uri[] mediaStoreImageUris = new Uri[] {
                MediaStore.Images.Media.EXTERNAL_CONTENT_URI,
                MediaStore.Images.Media.getContentUri("phoneStorage"),
        };
        String[] mediaProjection = new String[] {MediaStore.Images.ImageColumns.DATE_TAKEN,
                MediaStore.MediaColumns.MIME_TYPE, MediaStore.MediaColumns.DATA,
                MediaStore.MediaColumns._ID};

        // Approach 2: query MediaStore
        for (Uri storeUri : mediaStoreImageUris) {
            Cursor cursor = null;
            try {
                cursor = ContextUtils.getApplicationContext().getContentResolver().query(
                        storeUri, mediaProjection, null, null, null);
            } catch (SecurityException se) {
                // This happens on some exotic devices.
                Log.e(TAG, "Cannot query media store.", se);
            }

            if (cursor == null) {
                continue;
            }

            try {
                while (cursor.moveToNext()) {
                    String mimeType = cursor.getString(
                            cursor.getColumnIndexOrThrow(MediaStore.MediaColumns.MIME_TYPE));
                    String path = cursor.getString(
                            cursor.getColumnIndexOrThrow(MediaStore.MediaColumns.DATA));
                    int index = path.indexOf("Screenshot");
                    if (index != -1) {
                        File dir = new File(path.substring(0, index + 12));
                        screenshotDirs.add(dir);
                    }
                }
            } finally {
                cursor.close();
            }
        }
    }

    /**
     * This class requires the caller (ScreenshotMonitor) to call
     * @{link ScreenshotMonitorFileObserver#setScreenshotMonitor(ScreenshotMonitor)} after
     * instantiation of the class.
     */
    @VisibleForTesting
    static class ScreenshotMonitorFileObserver extends FileObserver {
        // TODO(angelashao): Generate screenshot directory path based on device (crbug/734220).

        // This can only be accessed on the UI thread.
        private ScreenshotMonitor mScreenshotMonitor;

        /**
         * Creates a FileObserver that monitors the directory with {@code directoryPath}.
         * @param directoryPath
         */
        public ScreenshotMonitorFileObserver(String directoryPath) {
            super(directoryPath, FileObserver.CREATE);
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

    @VisibleForTesting
    ScreenshotMonitor(ScreenshotMonitorDelegate delegate,
            ArrayList<ScreenshotMonitorFileObserver> fileObservers) {
        mDelegate = delegate;
        mFileObservers = fileObservers;

        for (ScreenshotMonitorFileObserver observer : mFileObservers) {
            observer.setScreenshotMonitor(this);
        }
    }

    /**
     * Create an instance of {@link ScreenshotMonitor}. If no external storage is available to
     * monitor, return null.
     * @param delegate The {@link ScreenshotMonitorDelegate} to call when screenshot is detected.
     * @return An instance of {@link ScreenshotMonitor}.
     */
    public static ScreenshotMonitor create(ScreenshotMonitorDelegate delegate) {
        File storageRoot = getStorageRoot();
        if (storageRoot == null) return null;

        Set<File> screenshotDirs = new HashSet<File>();
        getScreenshotDirs(storageRoot, screenshotDirs);
        ArrayList<ScreenshotMonitorFileObserver> fileObservers =
                new ArrayList<ScreenshotMonitorFileObserver>();
        for (File dir : screenshotDirs) {
            fileObservers.add(new ScreenshotMonitorFileObserver(dir.getPath()));
        }
        return new ScreenshotMonitor(delegate, fileObservers);
    }

    /**
     * Start monitoring the screenshot directory.
     */
    public void startMonitoring() {
        ThreadUtils.assertOnUiThread();
        for (ScreenshotMonitorFileObserver observer : mFileObservers) {
            observer.startWatching();
        }
        mIsMonitoring = true;
    }

    /**
     * Stop monitoring the screenshot directory.
     */
    public void stopMonitoring() {
        ThreadUtils.assertOnUiThread();
        for (ScreenshotMonitorFileObserver observer : mFileObservers) {
            observer.stopWatching();
        }
        mIsMonitoring = false;
    }

    private void onEventOnUiThread(final int event, final String path) {
        if (!mIsMonitoring) return;
        if (path == null) return;
        assert event == FileObserver.CREATE;

        mDelegate.onScreenshotTaken();
    }
}
