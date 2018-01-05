// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.app.Activity;
import android.app.AlertDialog;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.content_public.browser.WebContents;

/**
 * Helper class to handle communication between download location dialog and native.
 */
public class DownloadLocationDialogHelper
        implements DownloadLocationDialog.DownloadLocationDialogListener {
    @CalledByNative
    public static DownloadLocationDialogHelper create() {
        return new DownloadLocationDialogHelper();
    }

    @CalledByNative
    public void showDialog(WebContents webContents, String suggestedPath) {
        Activity windowAndroidActivity = webContents.getTopLevelNativeWindow().getActivity().get();
        AlertDialog dialog = new DownloadLocationDialog(windowAndroidActivity, this, suggestedPath);
        dialog.show();
    }

    @Override
    public void onComplete(String returnedPath) {
        nativeOnComplete(returnedPath);
    }

    public native void nativeOnComplete(String returnedPath);
}
