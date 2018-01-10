// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.content.Context;
import android.support.v7.app.AlertDialog;
import android.util.TypedValue;
import android.view.ContextThemeWrapper;
import android.view.View;

import org.chromium.base.Log;
import org.chromium.chrome.R;

/**
 * Hides Android implementation of AlertDialog from Chrome.
 * Chrome must use this class instead of AlertDialog to gurantee that the UI
 * would be supported in VR.
 */
public class ChromeAlertDialog extends AlertDialog {
    private static final String TAG = "ChromeAlertDialog";

    protected View mView;
    protected static Class sDialogHandlerClass;
    protected DialogRenderer mDialogHandler;

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
    public static void setDialogHandler(java.lang.Class alertDialogClass) {
        sDialogHandlerClass = alertDialogClass;
    }

    @Override
    public void show() {
        if (sDialogHandlerClass == null) {
            super.show();
            return;
        }
        dialogHandler().setParent(this);
        dialogHandler().show();
    }

    @Override
    public void setView(View view) {
        mView = view;
        super.setView(mView);
        if (dialogHandler() != null) dialogHandler().setView(mView);
    }

    private AlertDialog getCurrentInstance() {
        return this;
    }

    private static int resolveDialogTheme(Context context, int resid) {
        TypedValue outValue = new TypedValue();
        context.getTheme().resolveAttribute(R.attr.alertDialogTheme, outValue, true);
        return outValue.resourceId;
    }

    @Override
    public void setButton(int whichButton, CharSequence text, OnClickListener listener) {
        super.setButton(whichButton, text, listener);
        if (dialogHandler() != null) {
            dialogHandler().setButton(whichButton, text, listener);
        }
    }

    @Override
    public void setOnDismissListener(OnDismissListener onDismissListener) {
        super.setOnDismissListener(onDismissListener);
        if (dialogHandler() != null) dialogHandler().setOnDismissListener(onDismissListener);
    }

    @Override
    public void dismiss() {
        if (dialogHandler() != null) {
            dialogHandler().dismiss();
        } else {
            super.dismiss();
        }
    }

    private DialogRenderer dialogHandler() {
        if (sDialogHandlerClass == null) mDialogHandler = null;
        try {
            if (sDialogHandlerClass != null && mDialogHandler == null) {
                Object object = sDialogHandlerClass.newInstance();
                assert object
                        instanceof DialogRenderer
                    : "DialogHandlerClass should extend DialogRenderer";
                mDialogHandler = (DialogRenderer) object;
            }
        } catch (InstantiationException e) {
            Log.e(TAG, "Failed to instantiate a new Dialog.", e);
            System.exit(-1);
        } catch (IllegalAccessException e) {
            Log.e(TAG, "Illegal access while instantiating a new Dialog.", e);
            System.exit(-1);
        }
        return mDialogHandler;
    }
}
