// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages;

import android.annotation.TargetApi;
import android.app.DownloadManager;
import android.content.Context;
import android.net.Uri;
import android.os.Build;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Since the ADM can only be accessed from Java, this bridge will transfer all C++ calls over to
 * Java land for making the call to ADM.  This is a one-way bridge, from C++ to Java only. The
 * Java side of this bridge is not called by other Java code.
 */
@JNINamespace("offline_pages::android")
public class OfflinePagesDownloadManagerBridge {
    /**
     * Checks to see if ADM is installed on this phone.
     * Returns true if the android download app is installed.
     */
    @CalledByNative
    private static boolean isAndroidDownloadManagerInstalled() {
        // Get the DownloadManager service.
        DownloadManager downloadManager = getDownloadManager();
        return (downloadManager != null);
    }

    /** This is a pass through to the ADM function of the same name. */
    @CalledByNative
    private static long addCompletedDownload(String title, String description, String path,
            long length, String uri, String referer) {
        // Offline pages should not be scanned as for media content.
        boolean isMediaScannerScannable = false;
        // We don't want another download notification, since we already made one.
        boolean showNotification = false;
        String mimeType = "text/mthml";
        // Call the proper version of the pass through based on the supported API level.
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N) {
            return callAddCompletedDownload(title, description, isMediaScannerScannable, mimeType,
                    path, length, showNotification);
        }

        return callAddCompletedDownloadNougat(title, description, isMediaScannerScannable, mimeType,
                path, length, showNotification, uri, referer);
    }

    // Use this pass through for API level 1 up through 23.
    private static long callAddCompletedDownload(String title, String description,
            boolean isMediaScannerScannable, String mimeType, String path, long length,
            boolean showNotification) {
        DownloadManager downloadManager = getDownloadManager();
        if (downloadManager == null) return 0;

        return downloadManager.addCompletedDownload(title, description, isMediaScannerScannable,
                mimeType, path, length, showNotification);
    }

    // Use this pass through for API levels 24 and higher.
    @TargetApi(Build.VERSION_CODES.N)
    private static long callAddCompletedDownloadNougat(String title, String description,
            boolean isMediaScannerScannable, String mimeType, String path, long length,
            boolean showNotification, String uri, String referer) {
        DownloadManager downloadManager = getDownloadManager();
        if (downloadManager == null) return 0;

        return downloadManager.addCompletedDownload(title, description, isMediaScannerScannable,
                mimeType, path, length, showNotification, Uri.parse(uri), Uri.parse(referer));
    }

    /** This is a pass through to the ADM function of the same name. */
    @CalledByNative
    private static int remove(long[] ids) {
        DownloadManager downloadManager = getDownloadManager();
        if (downloadManager == null) return 0;

        return downloadManager.remove(ids);
    }

    private static DownloadManager getDownloadManager() {
        return (DownloadManager) ContextUtils.getApplicationContext().getSystemService(
                Context.DOWNLOAD_SERVICE);
    }
}
