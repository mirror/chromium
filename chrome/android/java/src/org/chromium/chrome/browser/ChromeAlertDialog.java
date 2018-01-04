// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.content.Context;
import android.support.v7.app.AlertDialog;
import android.util.TypedValue;
import android.view.ContextThemeWrapper;
import android.view.View;

import org.chromium.chrome.R;

/**
 * Hides Android implementation of AlertDialog from Chrome.
 * Chrome must use this class instead of AlertDialog to gurantee that the UI
 * would be supported in VR.
 */
public class ChromeAlertDialog extends AlertDialog {
    private static final String TAG = "ChromeAlertDialog";
    private OnDismissListener mOnDismissListener;
    protected View mView;
    protected static DialogRenderer sDialogHandler;

    protected ChromeAlertDialog(Context context, int themeResId) {
        super(context, themeResId);
    }

    /**
     * Returns a new ChromeAlertDialog
     */
    public static ChromeAlertDialog getInstance(Context context, int themeResId) {
        ChromeAlertDialog dialog = new ChromeAlertDialog(
                new ContextThemeWrapper(context, resolveDialogTheme(context, themeResId)),
                themeResId);
        dialog.setCancelable(true);
        dialog.setCanceledOnTouchOutside(true);
        return dialog;
    }

    /**
     * If DialogRenderer is not empty, we stop showing AlertDialogs
     * and will ask sDailogHandler to show the dialogs.
     */
    public static void setDialogHandler(DialogRenderer alertDialog) {
        sDialogHandler = alertDialog;
    }

    @Override
    public void show() {
        if (sDialogHandler == null) {
            super.show();
            return;
        }
        sDialogHandler.setParent(this);
        sDialogHandler.show();
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

    @Override
    public void setOnDismissListener(OnDismissListener onDismissListener) {
        mOnDismissListener = onDismissListener;
        super.setOnDismissListener(onDismissListener);
    }

    @Override
    public void dismiss() {
        if (sDialogHandler != null) {
            mOnDismissListener.onDismiss(getCurrentInstance());
            sDialogHandler.dismiss();
        } else
            super.dismiss();
    }
}
