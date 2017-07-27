// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages.downloads;

import android.app.DownloadManager;
import android.content.Context;
import android.os.AsyncTask;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.offlinepages.OfflinePageItem;

@JNINamespace("offline_pages::android")
public class OfflinePageDownloadManagerBridge {
    private static final String MIME_TYPE = "text/html";

    @CalledByNative
    public static void addPage(final OfflinePageItem offlinePage, final long callbackPointer) {
        AsyncTask<Void, Void, Long> task = new AsyncTask<Void, Void, Long>() {
            @Override
            public Long doInBackground(Void... params) {
                return addPageToDownloadManager(offlinePage);
            }

            @Override
            protected void onPostExecute(Long result) {
                nativeRunAddPageCallback(callbackPointer, offlinePage.getOfflineId(), result);
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private static long addPageToDownloadManager(OfflinePageItem offlinePage) {
        assert !ThreadUtils.runningOnUiThread();

        DownloadManager manager =
                (DownloadManager) ContextUtils.getApplicationContext().getSystemService(
                        Context.DOWNLOAD_SERVICE);
        long downloadId = -1;

        try {
            downloadId = manager.addCompletedDownload(offlinePage.getFilePath(),
                    offlinePage.getFilePath(), true /* is media scannable */, MIME_TYPE,
                    offlinePage.getFilePath(), offlinePage.getFileSize(),
                    false /* Show system notification */);
        } catch (RuntimeException e) {
            // Add logging here.
            // Log.w(TAG, "Failed to add the download item to DownloadManager: ", e);
        }
        return downloadId;
    }

    private static native void nativeRunAddPageCallback(
            long callbackPointer, long offlineId, long systemDownloadId);
}
