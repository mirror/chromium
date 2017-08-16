// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.content.Context;
import android.view.View;
import android.support.v7.app.AlertDialog;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import android.view.ContextThemeWrapper;
import android.util.TypedValue;
import android.view.ViewGroup;
import android.widget.TextView;

import java.util.List;

public class ChromeAlertDialog extends AlertDialog {
    private static final String TAG = "ChromeAlertDialog";
    protected View mView;
    protected static DialogRenderer mDialogHandler;

    protected ChromeAlertDialog(Context context, int themeResId) {
        super(context, themeResId);
    }

    public static ChromeAlertDialog GetInstance(Context context, int themeResId) {
        ChromeAlertDialog dialog = new ChromeAlertDialog(
                new ContextThemeWrapper(context, resolveDialogTheme(context, themeResId)),
                themeResId);
        dialog.setCancelable(true);
        dialog.setCanceledOnTouchOutside(true);
        return dialog;
    }

    public static void setDialogHandler(DialogRenderer alertDialog) {
        mDialogHandler = alertDialog;
    }

    @Override
    public void show() {
        Log.e(TAG, "VRP: show");
        if (mDialogHandler != null)
            mDialogHandler.show();
        else
            super.show();
    }

    @Override
    public void setView(View view) {
        mView = view;
        super.setView(view);
        if (mDialogHandler != null) mDialogHandler.setView(mView);
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
        if (mDialogHandler != null) mDialogHandler.setButton(whichButton, text, listener);
    }
}
