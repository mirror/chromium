// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.content.Context;
import android.view.View;
import android.support.v7.app.AlertDialog;

import android.content.DialogInterface;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import android.view.ContextThemeWrapper;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.DialogRenderer;

import java.util.ArrayList;
import java.util.List;

@JNINamespace("vr_shell")
public class VrAlertDialog implements DialogRenderer {
    private static final String TAG = "VrDialogRenderer";
    private long mNativeVrShellDelegate;
    private long mNativeVrAlertDialog;

    private ArrayList<CharSequence> mTextMessages = new ArrayList<>();
    private ArrayList<Long> mIcons = new ArrayList<>();
    private ArrayList<Button> mButtons = new ArrayList<>();

    protected View mView;

    public VrAlertDialog(long vrShellDelegate) {
        mNativeVrShellDelegate = vrShellDelegate;
    }

    private class Button {
        private long mId;
        private String mText;
        private DialogInterface.OnClickListener mListener;
        Button(long id, String text, DialogInterface.OnClickListener listener) {
            // TODO make sure that input is valid
            mId = id;
            mText = text;
            mListener = listener;
        }

        public long getId() {
            return mId;
        }
        public String getText() {
            return mText;
        }
        public void callListerner() {}
    }

    @Override
    public boolean show() {
        // TODO validate the view

        if (mView instanceof ViewGroup)
            loopViews((ViewGroup) mView);
        else {
            Log.e(TAG, "not a group");
            return false;
        }
        // TODO Initialize corresponding nativeInitVrAlert based on the Title, Buttons, Theme
        if (mTextMessages.size() == 2 && mButtons.size() == 2) {
            mNativeVrAlertDialog = nativeInitVrAlert(mNativeVrShellDelegate, 0,
                    mTextMessages.get(0).toString(), mTextMessages.get(1).toString(),
                    DialogInterface.BUTTON_POSITIVE, mButtons.get(0).getText(),
                    DialogInterface.BUTTON_NEGATIVE, mButtons.get(1).getText());
            VrShellDelegate.setDialogView(mView, mButtons.get(0).getText());
            mTextMessages = new ArrayList<>();
            mButtons = new ArrayList<>();
            return true;
        } else {
            Log.e(TAG, "Dismiss! size " + mTextMessages.size());
            mTextMessages = new ArrayList<>();
            mButtons = new ArrayList<>();

            return false;
        }
    }

    @Override
    public void setView(View view) {
        mView = view;
    }

    private int resolveDialogTheme(Context context, int resid) {
        if (resid >= 0x01000000) { // start of real resource IDs.
            return resid;
        } else {
            TypedValue outValue = new TypedValue();
            context.getTheme().resolveAttribute(R.attr.alertDialogTheme, outValue, true);
            return outValue.resourceId;
        }
    }

    @Override
    public void setButton(
            int whichButton, CharSequence text, DialogInterface.OnClickListener listener) {
        // TODO validate the input
        mButtons.add(new Button(whichButton, text.toString(), listener));
    }

    private void loopViews(ViewGroup view) {
        for (int i = 0; i < view.getChildCount(); i++) {
            View v = view.getChildAt(i);
            if (v instanceof TextView) {
                // Do something
                CharSequence text = ((TextView) v).getText();
                if (text.length() > 0) mTextMessages.add(text);
            } else if (v instanceof ViewGroup) {
                loopViews((ViewGroup) v);
            }
        }
    }

    @CalledByNative
    public void OnButtonClicked(/*int  whichButton*/) {
        // switch (whichButton) callback
    }

    private native long nativeInitVrAlert(long vrShellDelegate, long icon, String titleText,
            String toggleMessage, int buttonPositive, String buttonPossitiveText,
            int buttonNegative, String buttonNegativeText);
}
