// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.app.Dialog;
import android.content.Context;
import android.os.Handler;
import android.util.TypedValue;
import android.view.ContextThemeWrapper;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.chrome.R;

public class ChromeDialog extends Dialog {
    private static final String TAG = "ChromeDialog";
    protected View mView;
    protected static DialogRenderer sDialogHandler;

    protected ChromeDialog(Context context, int themeResId) {
        super(context, themeResId);
    }

    public static ChromeDialog getInstance(Context context, int themeResId) {
        ChromeDialog dialog = new ChromeDialog(
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
    public void addContentView(View view, ViewGroup.LayoutParams params) {
        mView = view;
        if (sDialogHandler != null) {
            sDialogHandler.setView(mView);
        } else {
            super.addContentView(mView, params);
        }
    }

    private Dialog getCurrentInstance() {
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

    OnDismissListener mOnDismissListener;
    @Override
    public void setOnDismissListener(OnDismissListener onDismissListener) {
        mOnDismissListener = onDismissListener;
        super.setOnDismissListener(onDismissListener);
    }
    @Override
    public void dismiss() {
        if (sDialogHandler != null) {
            if (mOnDismissListener != null) mOnDismissListener.onDismiss(getCurrentInstance());
            sDialogHandler.dismiss();
        } else {
            super.dismiss();
        }
    }

    @Override
    public boolean isShowing() {
        if (sDialogHandler != null)
            return sDialogHandler.isShowing();
        else
            return super.isShowing();
    }
}
