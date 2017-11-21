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

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.JNINamespace;

import java.util.ArrayList;

@JNINamespace("vr_shell")
public class VrAlertDialog extends VrView {
    private static final String TAG = "VrAlertDialog";

    private ArrayList<DialogButton> mButtons = new ArrayList<>();
    protected LinearLayout mButtonLayout;
    protected Dialog mDialog;
    protected View mView;

    public VrAlertDialog(long vrShellDelegate) {
        super(vrShellDelegate);
    }

    private class DialogButton {
        private long mId;
        private String mText;
        private DialogInterface.OnClickListener mListener;
        DialogButton(long id, String text, DialogInterface.OnClickListener listener) {
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
        public DialogInterface.OnClickListener getListener() {
            return mListener;
        }
    }

    @Override
    public boolean show() {
        super.show();
        // TODO validate the view
        setLayout(new LinearLayout(mView.getContext()));

        mButtonLayout = new LinearLayout(mView.getContext());
        Button b = new Button(mView.getContext());
        b.setText(mButtons.get(0).getText());
        b.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        b.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mButtons.get(0).getListener().onClick(null, b.getId());
                VrShellDelegate.setDialogView(null, mButtons.get(0).getText());
                closeVrView();
                mDialog.dismiss();
            }
        });

        Button b2 = new Button(mView.getContext());
        b2.setText(mButtons.get(1).getText());
        b2.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        b2.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                // Code here executes on main thread after user presses button
                mButtons.get(1).getListener().onClick(null, b2.getId());
                VrShellDelegate.setDialogView(null, mButtons.get(0).getText());
                ThreadUtils.runOnUiThread(new Runnable() {
                    public void run() {
                        mDialog.dismiss();
                    }
                });

                closeVrView();
            }
        });

        mButtonLayout.setOrientation(LinearLayout.HORIZONTAL);
        mButtonLayout.addView(b);
        mButtonLayout.addView(b2);

        getLayout().setOrientation(LinearLayout.VERTICAL);
        LayoutParams params = new LinearLayout.LayoutParams(1100, 500);

        getLayout().addView(mView, params);
        getLayout().addView(mButtonLayout);

        VrShellDelegate.setDialogView(getLayout(), mButtons.get(0).getText());
        initVrView();

        return true;
    }

    @Override
    public void setView(View view) {
        mView = view;
    }

    @Override
    public void setParent(Dialog dialog) {
        mDialog = dialog;
    }

    @Override
    public void setButton(
            int whichButton, CharSequence text, DialogInterface.OnClickListener listener) {
        // TODO validate the input
        mButtons.add(new DialogButton(whichButton, text.toString(), listener));
    }
}
