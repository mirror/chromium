// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.os.AsyncTask;

import org.chromium.chrome.browser.metrics.WebApkUma;

/**
 * Java counterpart to webapk_install_space_manager.h
 * Contains functionality to check disk space and free cache.
 */
public class WebApkInstallSpaceManager {
    // Matches order in webapk_install_space_manager.h
    private static final int ENOUGH_SPACE = 0;
    private static final int ENOUGH_SPACE_AFTER_FREE_UP_CACHE = 1;
    private static final int NOT_ENOUGH_SPACE = 2;
    private static final int UNDERDETERMINED = 3;

    /**
     * Check the available space status and set it to native object.
     * @param nativeWebApkInstallSpaceManager The pointer to the native object.
     */
    public static void setSpaceStatus(long nativeWebApkInstallSpaceManager) {
        new AsyncTask<Void, Void, Void>() {
            long mAvailableSpaceInByte = 0;
            long mCacheSizeInByte = 0;
            @Override
            protected Void doInBackground(Void... params) {
                mAvailableSpaceInByte = WebApkUma.getAvailableSpaceAboveLowSpaceLimit();
                mCacheSizeInByte = WebApkUma.getCacheDirSize();
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                int spaceStatus = UNDERDETERMINED;
                if (mAvailableSpaceInByte > 0) {
                    spaceStatus = ENOUGH_SPACE;
                } else if (mAvailableSpaceInByte + mCacheSizeInByte > 0) {
                    spaceStatus = ENOUGH_SPACE_AFTER_FREE_UP_CACHE;
                } else {
                    spaceStatus = NOT_ENOUGH_SPACE;
                }

                nativeSetSpaceStatus(nativeWebApkInstallSpaceManager, spaceStatus);
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private static native void nativeSetSpaceStatus(
            long nativeWebApkInstallSpaceManager, int spaceStatus);
}
