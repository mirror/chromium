// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.support.v7.app.AlertDialog;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;

/**
 * A dialog to notify users that WebAPKs and Trusted Web Activities need a network connection to
 * launch.
 */

public class WebAppOfflineDialog {
    private Dialog mDialog;

    /**
     * Shows the dialog that notifies users that the WebAPK or TWA is offline.
     * @param activity Activity that will be used for {@link Dialog#show()}.
     * @param appName The name of the Android native client for which the dialog is shown.
     */
    public void show(final Activity activity, String appName) {
        AlertDialog.Builder builder = new AlertDialog.Builder(activity, R.style.AlertDialogTheme);
        builder.setMessage(activity.getString(R.string.webapk_offline_dialog, appName))
                .setNegativeButton(R.string.webapk_offline_dialog_quit_button,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                ApiCompatibilityUtils.finishAndRemoveTask(activity);
                            }
                        });

        mDialog = builder.create();
        mDialog.setCanceledOnTouchOutside(false);
        mDialog.show();
    }

    /** Closes the dialog. */
    public void cancel() {
        mDialog.cancel();
    }
}
