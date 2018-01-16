// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.app.Activity;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.content_public.browser.WebContents;

import java.io.File;

/**
 * Helper class to handle communication between download location dialog and native.
 */
public class DownloadLocationDialogHelper
        implements DownloadLocationDialog.DownloadLocationDialogListener {
    private long mNativeDownloadLocationDialogHelper;
    private DownloadLocationDialog mLocationDialog;

    private DownloadLocationDialogHelper(long nativeDownloadLocationDialogHelper) {
        mNativeDownloadLocationDialogHelper = nativeDownloadLocationDialogHelper;
    }

    @CalledByNative
    public static DownloadLocationDialogHelper create(long nativeDownloadLocationDialogHelper) {
        return new DownloadLocationDialogHelper(nativeDownloadLocationDialogHelper);
    }

    @CalledByNative
    private void destroy() {
        mNativeDownloadLocationDialogHelper = 0;
        if (mLocationDialog != null) mLocationDialog.dismiss();
    }

    @CalledByNative
    public void showDialog(WebContents webContents, String suggestedPath) {
        Activity windowAndroidActivity = webContents.getTopLevelNativeWindow().getActivity().get();
        if (windowAndroidActivity == null) return;

        if (mLocationDialog != null) return;
        mLocationDialog =
                new DownloadLocationDialog(windowAndroidActivity, this, new File(suggestedPath));
        mLocationDialog.show();
    }

    @Override
    public void onComplete(File returnedPath) {
        if (mNativeDownloadLocationDialogHelper == 0) return;

        nativeOnComplete(mNativeDownloadLocationDialogHelper, returnedPath.getAbsolutePath());
        mLocationDialog = null;
    }

    public native void nativeOnComplete(
            long nativeDownloadLocationDialogHelper, String returnedPath);
}
