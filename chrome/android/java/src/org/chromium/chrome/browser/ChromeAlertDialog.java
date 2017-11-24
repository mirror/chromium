// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.content.Context;
import android.os.Handler;
import android.support.v7.app.AlertDialog;
import android.util.TypedValue;
import android.view.ContextThemeWrapper;
import android.view.View;

import org.chromium.chrome.R;

public class ChromeAlertDialog extends AlertDialog {
    private static final String TAG = "ChromeAlertDialog";
    protected View mView;
    protected static DialogRenderer sDialogHandler;

    protected ChromeAlertDialog(Context context, int themeResId) {
        super(context, themeResId);
    }

    public static ChromeAlertDialog getInstance(Context context, int themeResId) {
        ChromeAlertDialog dialog = new ChromeAlertDialog(
                new ContextThemeWrapper(context, resolveDialogTheme(context, themeResId)),
                themeResId);
        dialog.setCancelable(true);
        dialog.setCanceledOnTouchOutside(true);
        return dialog;
    }

    public static void setDialogHandler(DialogRenderer alertDialog) {
        sDialogHandler = alertDialog;
    }

    @Override
    public void show() {
        if (sDialogHandler != null) {
            sDialogHandler.setParent(this);
            if (!sDialogHandler.show()) {
                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        mOnDismissListener.onDismiss(getCurrentInstance());
                    }
                }, 1000);
            }
        } else
            super.show();
    }

    @Override
    public void setView(View view) {
        mView = view;
        super.setView(mView);
        if (sDialogHandler != null) sDialogHandler.setView(mView);
    }

    private AlertDialog getCurrentInstance() {
        return this;
    }

    private static int resolveDialogTheme(Context context, int resid) {
        if (resid >= 0x01000000) { // start of real resource IDs.
            return resid;
        } else {
            TypedValue outValue = new TypedValue();
            context.getTheme().resolveAttribute(R.attr.alertDialogTheme, outValue, true);
            return outValue.resourceId;
        }
    }

    @Override
    public void setButton(int whichButton, CharSequence text, OnClickListener listener) {
        super.setButton(whichButton, text, listener);
        if (sDialogHandler != null) sDialogHandler.setButton(whichButton, text, listener);
    }

    OnDismissListener mOnDismissListener;
    @Override
    public void setOnDismissListener(OnDismissListener onDismissListener) {
        mOnDismissListener = onDismissListener;
        super.setOnDismissListener(onDismissListener);
    }
    @Override
    public void dismiss() {
        if (sDialogHandler != null)
            mOnDismissListener.onDismiss(getCurrentInstance());
        else
            super.dismiss();
    }
}
