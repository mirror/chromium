// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.password;

import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.v7.app.AlertDialog;
import android.view.View;
import android.widget.TextView;

import org.chromium.chrome.R;

/**
 * Shows the dialog that explains the user the error which just happened during exporting and
 * optionally helps them to take actions to fix that (learning more, retrying export).
 */
public class ExportErrorDialogFragment extends DialogFragment {
    // The key for the argument with the resource ID of the string to label the positive button.
    // This needs to always be passed in the arguments.
    public static final String POSITIVE_BUTTON_LABEL = "positive-button-label";

    // The key for the argument with the dialog description string. This needs to always be passed
    // in the arguments.
    public static final String DESCRIPTION = "description";

    // The key for the argument with the detailed description string for the dialog, displayed below
    // the main description. This is an optional part of the arguments.
    public static final String DETAILED_DESCRIPTION = "detailed-description";

    // This handler is used to answer the user actions on the dialog.
    private DialogInterface.OnClickListener mHandler;

    public void setExportErrorHandler(DialogInterface.OnClickListener handler) {
        mHandler = handler;
    }

    /**
     * Opens the dialog with the warning and sets the button listener to a fragment identified by ID
     * passed in arguments.
     */
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final View dialog =
                getActivity().getLayoutInflater().inflate(R.layout.passwords_error_dialog, null);
        final TextView mainDescription =
                (TextView) dialog.findViewById(R.id.passwords_error_main_description);
        mainDescription.setText(getArguments().getString(DESCRIPTION));
        final TextView detailedDescription =
                (TextView) dialog.findViewById(R.id.passwords_error_detailed_description);
        if (getArguments().containsKey(DETAILED_DESCRIPTION)) {
            detailedDescription.setText(getArguments().getString(DETAILED_DESCRIPTION));
        } else {
            detailedDescription.setVisibility(View.GONE);
        }
        return new AlertDialog.Builder(getActivity(), R.style.SimpleDialog)
                .setView(dialog)
                .setTitle(R.string.save_password_preferences_export_error_title)
                .setPositiveButton(getArguments().getInt(POSITIVE_BUTTON_LABEL), mHandler)
                .setNegativeButton(R.string.close, mHandler)
                .create();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // TODO(vabr): Figure out how to handle background killing.
        // If there is savedInstanceState, then the dialog is being recreated by Android and will
        // lack the necessary callbacks. Dismiss immediately as the settings page will need to be
        // recreated anyway.
        if (savedInstanceState != null) {
            dismiss();
            return;
        }
    }
}
