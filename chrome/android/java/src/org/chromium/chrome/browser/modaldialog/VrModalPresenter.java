// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modaldialog;

import android.app.Activity;
import android.text.InputType;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.FrameLayout;
import android.widget.TextView;

import org.chromium.chrome.browser.vr_shell.VrDialog;
import org.chromium.chrome.browser.vr_shell.VrShellDelegate;

/** The presenter that shows a {@link ModalDialogView} in an Android dialog. */
public class VrModalPresenter extends ModalDialogManager.Presenter {
    private final Activity mActivity;
    private VrDialog mVrDialog;
    private static final int DIALOG_WIDTH = 1200;

    public VrModalPresenter(Activity activity) {
        mActivity = activity;
    }

    @Override
    protected void addDialogView(View dialogView) {
        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                MarginLayoutParams.MATCH_PARENT, MarginLayoutParams.WRAP_CONTENT, Gravity.CENTER);

        mVrDialog = new VrDialog();
        mVrDialog.setLayout(new FrameLayout(dialogView.getContext()));
        mVrDialog.getLayout().setLayoutParams(
                new FrameLayout.LayoutParams(DIALOG_WIDTH, ViewGroup.LayoutParams.WRAP_CONTENT));
        mVrDialog.getLayout().addView(dialogView, params);
        disbaleSoftKeyboard(mVrDialog.getLayout());
        VrShellDelegate.setDialogView(mVrDialog, mVrDialog.getLayout());
        mVrDialog.initVrDialog();
    }

    @Override
    protected void removeDialogView(View dialogView) {
        // Dismiss the currently showing dialog.
        if (mVrDialog != null) mVrDialog.dismiss();
        mVrDialog = null;
    }

    private void disbaleSoftKeyboard(ViewGroup viewGroup) {
        for (int i = 0; i < viewGroup.getChildCount(); i++) {
            View view = viewGroup.getChildAt(i);
            if (view instanceof ViewGroup)
                disbaleSoftKeyboard((ViewGroup) view);
            else if (view instanceof TextView) {
                ((TextView) view).setInputType(InputType.TYPE_NULL);
            }
        }
        return;
    }
}
