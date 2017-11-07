// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.app.Activity;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.modaldialog.ModalDialog;
import org.chromium.chrome.browser.modaldialog.javascript.JavascriptModalDialog;
import org.chromium.ui.base.WindowAndroid;

/**
 * The controller to communicate with native JavaScriptDialogAndroid for a tab modal JavaScript
 * dialog. This can be an alert dialog, a prompt dialog or a confirm dialog.
 */
public class JavascriptTabModalDialog implements ModalDialog.TabModalController {
    private static final String TAG = "JsTabModalDialog";

    private final String mTitle;
    private final String mMessage;
    private final int mPositiveButtonTextId;
    private final int mNegativeButtonTextId;

    private String mDefaultPromptText;
    private long mNativeDialogPointer;
    private JavascriptModalDialog mDialog;

    /**
     * Constructor for initializing contents to be shown on the dialog.
     */
    private JavascriptTabModalDialog(
            String title, String message, int positiveButtonTextId, int negativeButtonTextId) {
        mTitle = title;
        mMessage = message;
        mPositiveButtonTextId = positiveButtonTextId;
        mNegativeButtonTextId = negativeButtonTextId;
    }

    /**
     * Constructor for creating prompt dialog only.
     */
    private JavascriptTabModalDialog(String title, String message, String defaultPromptText) {
        this(title, message, R.string.ok, R.string.cancel);
        mDefaultPromptText = defaultPromptText;
    }

    @CalledByNative
    private static JavascriptTabModalDialog createAlertDialog(String title, String message) {
        return new JavascriptTabModalDialog(title, message, R.string.ok, 0);
    }

    @CalledByNative
    private static JavascriptTabModalDialog createConfirmDialog(String title, String message) {
        return new JavascriptTabModalDialog(title, message, R.string.ok, R.string.cancel);
    }

    @CalledByNative
    private static JavascriptTabModalDialog createPromptDialog(
            String title, String message, String defaultPromptText) {
        return new JavascriptTabModalDialog(title, message, defaultPromptText);
    }

    @CalledByNative
    private void showDialog(WindowAndroid window, long nativeDialogPointer) {
        assert window != null;
        Activity activity = window.getActivity().get();
        // If the activity has gone away, then just clean up the native pointer.
        if (activity == null) {
            nativeCancel(nativeDialogPointer);
            return;
        }

        // Cache the native dialog pointer so that we can use it to return the response.
        mNativeDialogPointer = nativeDialogPointer;

        mDialog = new JavascriptModalDialog(activity, this);
        mDialog.setTitle(mTitle);
        mDialog.setMessage(mMessage);
        if (mPositiveButtonTextId != 0) mDialog.setPositiveButton(mPositiveButtonTextId);
        if (mNegativeButtonTextId != 0) mDialog.setNegativeButton(mNegativeButtonTextId);
        mDialog.setPromptText(mDefaultPromptText);
        mDialog.show();
    }

    @CalledByNative
    private String getUserInput() {
        return mDialog.getPromptText();
    }

    @CalledByNative
    private void dismiss() {
        mDialog.dismiss();
        mNativeDialogPointer = 0;
    }

    @Override
    public void onClick(int buttonType) {
        switch (buttonType) {
            case ModalDialog.BUTTON_POSITIVE:
                accept(mDialog.getPromptText());
                mDialog.dismiss();
                break;
            case ModalDialog.BUTTON_NEGATIVE:
                cancel();
                mDialog.dismiss();
                break;
            default:
                Log.e(TAG, "Unexpected button pressed in dialog: " + buttonType);
        }
    }

    @Override
    public void onDismissNoAction() {
        cancel();
    }

    /**
     * Sends notification to native that the user accepts the dialog.
     * @param promptResult The text edited by user.
     */
    private void accept(String promptResult) {
        if (mNativeDialogPointer != 0) {
            nativeAccept(mNativeDialogPointer, promptResult);
        }
    }

    /**
     * Sends notification to native that the user cancels the dialog.
     */
    private void cancel() {
        if (mNativeDialogPointer != 0) {
            nativeCancel(mNativeDialogPointer);
        }
    }

    /**
     * Returns the {@link ModalDialog} associated with this JavascriptTabModalDialog.
     */
    @VisibleForTesting
    public JavascriptModalDialog getDialogForTest() {
        return mDialog;
    }

    private native void nativeAccept(long nativeJavaScriptDialogAndroid, String prompt);
    private native void nativeCancel(long nativeJavaScriptDialogAndroid);
}
