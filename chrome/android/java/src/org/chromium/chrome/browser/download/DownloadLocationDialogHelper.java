// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.app.Activity;
import android.support.v7.app.AlertDialog;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.content_public.browser.WebContents;

/**
 * Helper class to handle communication between download location dialog and native.
 */
public class DownloadLocationDialogHelper
        implements DownloadLocationDialog.DownloadLocationDialogListener {
    private long mNativeDownloadLocationDialogHelper;

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
    }

    @CalledByNative
    public void showDialog(WebContents webContents, String suggestedPath) {
        Activity windowAndroidActivity = webContents.getTopLevelNativeWindow().getActivity().get();
        AlertDialog dialog = new DownloadLocationDialog(windowAndroidActivity, this, suggestedPath);
        dialog.show();
    }

    @Override
    public void onComplete(String returnedPath) {
        if (mNativeDownloadLocationDialogHelper == 0) return;

        nativeOnComplete(mNativeDownloadLocationDialogHelper, returnedPath);
    }

    public native void nativeOnComplete(
            long nativeDownloadLocationDialogHelper, String returnedPath);
}
