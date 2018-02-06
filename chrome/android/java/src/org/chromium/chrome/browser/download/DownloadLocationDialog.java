// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static org.chromium.chrome.browser.preferences.download.DownloadDirectoryPreference.FROM_DOWNLOAD_LOCATION_DIALOG;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.PreferencesLauncher;
import org.chromium.chrome.browser.preferences.download.DownloadDirectoryPreference;
import org.chromium.chrome.browser.preferences.download.DownloadDirectoryUtil;
import org.chromium.chrome.browser.widget.AlertDialogEditText;

import java.io.File;

/**
 * Dialog that is displayed to ask user where they want to download the file.
 */
public class DownloadLocationDialog
        extends AlertDialog implements DialogInterface.OnClickListener, View.OnClickListener {
    private DownloadLocationDialogListener mListener;
    private DownloadDirectoryUtil mDownloadDirectoryUtil;

    private AlertDialogEditText mFileName;
    private AlertDialogEditText mFileLocation;
    private CheckBox mShowAgain;

    /**
     * Interface for a listener to the events related to the DownloadLocationDialog.
     */
    public interface DownloadLocationDialogListener {
        /**
         * Notify listeners that the dialog has been completed.
         * @param returnedPath from the dialog.
         */
        void onComplete(File returnedPath);

        /**
         * Notify listeners that the dialog has been canceled.
         */
        void onCanceled();
    }

    /**
     * Create a DownloadLocationDialog that is displayed before a download begins.
     *
     * @param context of the dialog.
     * @param listener to the updates from the dialog.
     * @param suggestedPath of the download location.
     */
    DownloadLocationDialog(
            Context context, DownloadLocationDialogListener listener, File suggestedPath) {
        super(context, 0);

        mListener = listener;
        mDownloadDirectoryUtil = new DownloadDirectoryUtil(context);

        setButton(
                BUTTON_POSITIVE, context.getText(R.string.download_location_dialog_positive), this);
        setButton(BUTTON_NEGATIVE, context.getText(R.string.cancel), this);
        setIcon(0);
        setTitle(context.getString(R.string.download_location_dialog_title));

        LayoutInflater inflater = LayoutInflater.from(context);
        View view = inflater.inflate(R.layout.download_location_dialog, null);

        mFileName = (AlertDialogEditText) view.findViewById(R.id.file_name);
        mFileName.setText(suggestedPath.getName());

        mFileLocation = (AlertDialogEditText) view.findViewById(R.id.file_location);
        mFileLocation.setKeyListener(null);
        mFileLocation.setOnClickListener(this);
        setFileLocation(suggestedPath.getParentFile());

        mShowAgain = (CheckBox) view.findViewById(R.id.show_again_checkbox);
        boolean shouldShowAgain = PrefServiceBridge.getInstance().getBoolean(
                Pref.PROMPT_FOR_DOWNLOAD_ANDROID_INITIAL);
        mShowAgain.setChecked(shouldShowAgain);

        setView(view);
    }

    /**
     * Update the string in the file location text view.
     *
     * @param location  The location that the download will go to.
     */
    void setFileLocation(File location) {
        if (mFileLocation == null) return;
        mFileLocation.setText(mDownloadDirectoryUtil.getNameForFile(location));
    }

    // DialogInterface.OnClickListener implementation.

    @Override
    public void onClick(DialogInterface dialogInterface, int which) {
        if (which == BUTTON_POSITIVE) {
            // Get new file path information.
            String fileName = mFileName.getText().toString();
            File fileLocation =
                    mDownloadDirectoryUtil.getFileForName(mFileLocation.getText().toString());
            mListener.onComplete(new File(fileLocation, fileName));

            // Get checkbox ("show again") information.
            if (PrefServiceBridge.getInstance().getBoolean(
                        Pref.PROMPT_FOR_DOWNLOAD_ANDROID_INITIAL)) {
                PrefServiceBridge.getInstance().setBoolean(
                        Pref.PROMPT_FOR_DOWNLOAD_ANDROID_INITIAL, false);
            }

            PrefServiceBridge.getInstance().setBoolean(
                    Pref.PROMPT_FOR_DOWNLOAD_ANDROID, !mShowAgain.isChecked());
        } else {
            mListener.onCanceled();
        }

        dismiss();
    }

    // View.OnClickListener implementation.

    @Override
    public void onClick(View view) {
        // When the file location text view is clicked.
        Intent intent = PreferencesLauncher.createIntentForSettingsPage(
                getContext(), DownloadDirectoryPreference.class.getName());
        intent.putExtra(FROM_DOWNLOAD_LOCATION_DIALOG, true);
        getContext().startActivity(intent);
    }
}
