// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.app.Dialog;
import android.content.DialogInterface;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import org.chromium.base.Log;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.modaldialog.ModalDialogView;

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
    private ModalDialogView mModalDialogView;
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
        ModalDialogView.Controller controller = new ModalDialogView.Controller() {
            @Override
            public void onCancel() {}

            @Override
            public void onClick(int buttonType) {
                switch (buttonType) {
                    case ModalDialogView.BUTTON_POSITIVE:
                    case ModalDialogView.BUTTON_NEGATIVE:
                        mButtons.get(buttonType)
                                .getListener()
                                .onClick(null, mButtons.get(buttonType).getId());
                        VrShellDelegate.setDialogView(null);
                        mDialog.dismiss();
                        break;
                    default:
                        Log.e(TAG, "Unknown button type: " + buttonType);
                }
            }
        };
        final ModalDialogView.Params p = new ModalDialogView.Params();
        p.positiveButtonText = mButtons.get(ModalDialogView.BUTTON_POSITIVE).getText();
        p.negativeButtonText = mButtons.get(ModalDialogView.BUTTON_NEGATIVE).getText();
        LinearLayout ll = new LinearLayout(mView.getContext());
        ll.setLayoutParams(
                new LinearLayout.LayoutParams(DIALOG_WIDTH, ViewGroup.LayoutParams.WRAP_CONTENT));
        ll.addView(mView);
        p.customView = ll;
        mModalDialogView = new ModalDialogView(controller, p);

        setLayout(new LinearLayout(mView.getContext()));
        getLayout().setLayoutParams(
                new LinearLayout.LayoutParams(DIALOG_WIDTH, ViewGroup.LayoutParams.WRAP_CONTENT));
        getLayout().addView(mModalDialogView.getView());
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
