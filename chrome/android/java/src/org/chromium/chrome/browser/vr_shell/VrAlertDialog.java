// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.app.Dialog;
import android.content.DialogInterface;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;

import org.chromium.base.annotations.JNINamespace;

import java.util.ArrayList;

/**
 * VR handler for alert dialogs. It stops showing the Android AlertDialog
 * and builds a view consisting of buttons and view of the AlertDialog.
 * The view is used to draw the dialog on a quad in VR.
 */
@JNINamespace("vr_shell")
public class VrAlertDialog extends VrDialog {
    private static final String TAG = "VrAlertDialog";

    private ArrayList<DialogButton> mButtons = new ArrayList<>();
    protected LinearLayout mButtonLayout;
    protected Dialog mDialog;
    protected View mView;
    private static final int DIALOG_WIDTH = 1200;
    private class DialogButton {
        private int mId;
        private String mText;
        private DialogInterface.OnClickListener mListener;
        DialogButton(int id, String text, DialogInterface.OnClickListener listener) {
            mId = id;
            mText = text;
            mListener = listener;
        }

        public int getId() {
            return mId;
        }
        public String getText() {
            return mText;
        }
        public DialogInterface.OnClickListener getListener() {
            return mListener;
        }
    }

    /**
     * Builds a layout that contains view and buttons from AlertDialog.
     * Initiates the dialog in VR.
     */
    @Override
    public boolean show() {
        super.show();
        // TODO validate the view
        setLayout(new LinearLayout(mView.getContext()));
        getLayout().setLayoutParams(
                new LinearLayout.LayoutParams(DIALOG_WIDTH, ViewGroup.LayoutParams.WRAP_CONTENT));

        mButtonLayout = new LinearLayout(mView.getContext());
        Button button = new Button(mView.getContext());
        button.setText(mButtons.get(0).getText());
        button.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        button.setOnClickListener((View v) -> {
            mButtons.get(0).getListener().onClick(null, mButtons.get(0).getId());
            VrShellDelegate.setDialogView(null);
            mDialog.dismiss();
        });
        mButtonLayout.setOrientation(LinearLayout.HORIZONTAL);
        mButtonLayout.addView(button);
        button = new Button(mView.getContext());
        button.setText(mButtons.get(1).getText());
        button.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        button.setOnClickListener((View v) -> {
            mButtons.get(1).getListener().onClick(null, mButtons.get(1).getId());
            VrShellDelegate.setDialogView(null);
            mDialog.dismiss();
        });

        mButtonLayout.addView(button);

        getLayout().setOrientation(LinearLayout.VERTICAL);
        LayoutParams params =
                new LinearLayout.LayoutParams(1200, ViewGroup.LayoutParams.WRAP_CONTENT);

        getLayout().addView(mView, params);
        getLayout().addView(mButtonLayout);

        VrShellDelegate.setDialogView(getLayout());
        initVrDialog();

        return true;
    }

    /**
     * Set the main view
     */
    @Override
    public void setView(View view) {
        mView = view;
    }

    /**
     * Set the parent dialog. mDialog is used for dismissing the Dialog.
     */
    @Override
    public void setParent(Dialog dialog) {
        mDialog = dialog;
    }

    /**
     * Add button to the list of buttons of this Dialog.
     */
    @Override
    public void setButton(
            int whichButton, CharSequence text, DialogInterface.OnClickListener listener) {
        // TODO validate the input
        mButtons.add(new DialogButton(whichButton, text.toString(), listener));
    }
}
