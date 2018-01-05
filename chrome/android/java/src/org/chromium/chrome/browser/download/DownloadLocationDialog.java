// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.view.LayoutInflater;
import android.view.View;

import org.chromium.base.Log;
import org.chromium.chrome.R;

/**
 * Dialog that is displayed to ask user where they want to download the file.
 */
public class DownloadLocationDialog extends AlertDialog implements DialogInterface.OnClickListener {
    private DownloadLocationDialogListener mListener;

    public interface DownloadLocationDialogListener { void onComplete(); }

    public DownloadLocationDialog(Context context, DownloadLocationDialogListener listener) {
        super(context, 0);

        mListener = listener;

        setButton(BUTTON_POSITIVE, context.getText(android.R.string.ok), this);
        setButton(BUTTON_NEGATIVE, context.getText(android.R.string.cancel), this);
        setIcon(0);
        setTitle("Ready to download?");

        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.layout.download_location_dialog, null);
        setView(view);
    }

    @Override
    public void onClick(DialogInterface dialogInterface, int which) {
        Log.e("joy", "onClick: " + which);
        mListener.onComplete();
        dismiss();
    }
}
