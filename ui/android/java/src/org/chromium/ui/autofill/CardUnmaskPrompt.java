// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.autofill;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Handler;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;

import org.chromium.ui.R;

/**
 * A prompt that bugs users to enter their CVC when unmasking a Wallet instrument (credit card).
 */
public class CardUnmaskPrompt implements DialogInterface.OnDismissListener, TextWatcher {
    private CardUnmaskPromptDelegate mDelegate;
    private AlertDialog mDialog;

    /**
     * An interface to handle the interaction with an CardUnmaskPrompt object.
     */
    public interface CardUnmaskPromptDelegate {
        /**
         * Called when the dialog has been dismissed.
         */
        void dismissed();

        /**
         * Returns whether |userResponse| represents a valid value.
         */
        boolean checkUserInputValidity(String userResponse);

        /**
         * Called when the user has entered a value and pressed "verify".
         * @param userResponse The value the user entered (a CVC), or an empty string if the
         *        user canceled.
         */
        void onUserInput(String userResponse);
    }

    public CardUnmaskPrompt(
            Context context, CardUnmaskPromptDelegate delegate, String title, String instructions) {
        mDelegate = delegate;

        LayoutInflater inflater = LayoutInflater.from(context);
        View v = inflater.inflate(R.layout.autofill_card_unmask_prompt, null);
        ((TextView) v.findViewById(R.id.instructions)).setText(instructions);

        mDialog = new AlertDialog.Builder(context)
                          .setTitle(title)
                          .setView(v)
                          .setNegativeButton(R.string.cancel, null)
                          .setPositiveButton(R.string.card_unmask_confirm_button, null)
                          .setOnDismissListener(this)
                          .create();
    }

    public void show() {
        mDialog.show();

        // Override the View.OnClickListener so that pressing the positive button doesn't dismiss
        // the dialog.
        Button verifyButton = mDialog.getButton(AlertDialog.BUTTON_POSITIVE);
        verifyButton.setEnabled(false);
        verifyButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mDelegate.onUserInput(cardUnmaskInput().getText().toString());
            }
        });

        final EditText input = cardUnmaskInput();
        input.addTextChangedListener(this);
        input.post(new Runnable() {
            @Override
            public void run() {
                showKeyboardForUnmaskInput();
            }
        });
    }

    public void dismiss() {
        mDialog.dismiss();
    }

    public void disableAndWaitForVerification() {
        cardUnmaskInput().setEnabled(false);
        mDialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(false);

        getVerificationProgressBar().setVisibility(View.VISIBLE);
        getVerificationView().setVisibility(View.GONE);
    }

    public void verificationFinished(boolean success) {
        getVerificationProgressBar().setVisibility(View.GONE);
        if (!success) {
            TextView message = getVerificationView();
            message.setText("Verification failed. Please try again.");
            message.setVisibility(View.VISIBLE);
            EditText input = cardUnmaskInput();
            input.setEnabled(true);
            showKeyboardForUnmaskInput();
            // TODO(estade): UI decision - should we clear the input?
        } else {
            mDialog.findViewById(R.id.verification_success).setVisibility(View.VISIBLE);
            Handler h = new Handler();
            h.postDelayed(new Runnable() {
                public void run() {
                    dismiss();
                }
            }, 500);
        }
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        mDelegate.dismissed();
    }

    @Override
    public void afterTextChanged(Editable s) {
        boolean valid = mDelegate.checkUserInputValidity(cardUnmaskInput().getText().toString());
        mDialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(valid);
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) {}

    private void showKeyboardForUnmaskInput() {
        InputMethodManager imm = (InputMethodManager) mDialog.getContext().getSystemService(
                Context.INPUT_METHOD_SERVICE);
        imm.showSoftInput(cardUnmaskInput(), InputMethodManager.SHOW_IMPLICIT);
    }

    private EditText cardUnmaskInput() {
        return (EditText) mDialog.findViewById(R.id.card_unmask_input);
    }

    private ProgressBar getVerificationProgressBar() {
        return (ProgressBar) mDialog.findViewById(R.id.verification_progress_bar);
    }

    private TextView getVerificationView() {
        return (TextView) mDialog.findViewById(R.id.verification_message);
    }
}
