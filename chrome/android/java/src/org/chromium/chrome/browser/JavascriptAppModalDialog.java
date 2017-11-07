// Copyright 2012 The Chromium Authors. All rights reserved.
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
 * A dialog shown via JavaScript. This can be an alert dialog, a prompt dialog, a confirm dialog,
 * or an onbeforeunload dialog.
 */
public class JavascriptAppModalDialog implements ModalDialog.Controller {
    private static final String TAG = "JsAppModalDialog";

    private final String mTitle;
    private final String mMessage;
    private final int mPositiveButtonTextId;
    private final int mNegativeButtonTextId;
    private final boolean mShouldShowSuppressCheckBox;
    private String mDefaultPromptText;
    private long mNativeDialogPointer;
    private JavascriptModalDialog mDialog;

    /**
     * Constructor for initializing contents to be shown on the dialog.
     */
    private JavascriptAppModalDialog(String title, String message,
            int positiveButtonTextId, int negativeButtonTextId,
            boolean shouldShowSuppressCheckBox) {
        mTitle = title;
        mMessage = message;
        mPositiveButtonTextId = positiveButtonTextId;
        mNegativeButtonTextId = negativeButtonTextId;
        mShouldShowSuppressCheckBox = shouldShowSuppressCheckBox;
    }

    /**
     * Constructor for creating prompt dialog only.
     */
    private JavascriptAppModalDialog(String title, String message,
            boolean shouldShowSuppressCheckBox, String defaultPromptText) {
        this(title, message, R.string.ok, R.string.cancel, shouldShowSuppressCheckBox);
        mDefaultPromptText = defaultPromptText;
    }

    @CalledByNative
    public static JavascriptAppModalDialog createAlertDialog(String title, String message,
            boolean shouldShowSuppressCheckBox) {
        return new JavascriptAppModalDialog(title, message, R.string.ok, 0,
                shouldShowSuppressCheckBox);
    }

    @CalledByNative
    public static JavascriptAppModalDialog createConfirmDialog(String title, String message,
            boolean shouldShowSuppressCheckBox) {
        return new JavascriptAppModalDialog(title, message, R.string.ok, R.string.cancel,
                shouldShowSuppressCheckBox);
    }

    @CalledByNative
    public static JavascriptAppModalDialog createBeforeUnloadDialog(String title, String message,
            boolean isReload, boolean shouldShowSuppressCheckBox) {
        return new JavascriptAppModalDialog(title, message,
                isReload ? R.string.reload : R.string.leave, R.string.cancel,
                shouldShowSuppressCheckBox);
    }

    @CalledByNative
    public static JavascriptAppModalDialog createPromptDialog(String title, String message,
            boolean shouldShowSuppressCheckBox, String defaultPromptText) {
        return new JavascriptAppModalDialog(
                title, message, shouldShowSuppressCheckBox, defaultPromptText);
    }

    @CalledByNative
    void showJavascriptAppModalDialog(WindowAndroid window, long nativeDialogPointer) {
        assert window != null;
        Activity activity = window.getActivity().get();
        // If the activity has gone away, then just clean up the native pointer.
        if (activity == null) {
            nativeDidCancelAppModalDialog(nativeDialogPointer, false);
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
        mDialog.setSuppressCheckBoxVisibility(mShouldShowSuppressCheckBox);
        mDialog.show();
    }

    @Override
    public void onClick(@ModalDialog.ButtonType int buttonType) {
        switch (buttonType) {
            case ModalDialog.BUTTON_POSITIVE:
                confirm(mDialog.getPromptText(), mDialog.isSuppressCheckBoxChecked());
                mDialog.dismiss();
                break;
            case ModalDialog.BUTTON_NEGATIVE:
                cancel(mDialog.isSuppressCheckBoxChecked());
                mDialog.dismiss();
                break;
            default:
                Log.e(TAG, "Unexpected button pressed in dialog: " + buttonType);
        }
    }

    private void confirm(String promptResult, boolean suppressDialogs) {
        if (mNativeDialogPointer != 0) {
            nativeDidAcceptAppModalDialog(mNativeDialogPointer, promptResult, suppressDialogs);
        }
    }

    private void cancel(boolean suppressDialogs) {
        if (mNativeDialogPointer != 0) {
            nativeDidCancelAppModalDialog(mNativeDialogPointer, suppressDialogs);
        }
    }

    @CalledByNative
    private void dismiss() {
        mDialog.dismiss();
        mNativeDialogPointer = 0;
    }

    /**
     * Returns the currently showing dialog, null if none is showing.
     */
    @VisibleForTesting
    public static JavascriptAppModalDialog getCurrentDialogForTest() {
        return nativeGetCurrentModalDialog();
    }

    /**
     * Returns the {@link ModalDialog} associated with this JavascriptAppModalDialog.
     */
    @VisibleForTesting
    public JavascriptModalDialog getDialogForTest() {
        return mDialog;
    }

    private native void nativeDidAcceptAppModalDialog(long nativeJavascriptAppModalDialogAndroid,
            String prompt, boolean suppress);
    private native void nativeDidCancelAppModalDialog(long nativeJavascriptAppModalDialogAndroid,
            boolean suppress);
    private static native JavascriptAppModalDialog nativeGetCurrentModalDialog();
}
