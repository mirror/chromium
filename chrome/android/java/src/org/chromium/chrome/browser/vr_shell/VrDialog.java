// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.text.InputType;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.TextView;

/**
 * Manages how dialogs look inside of VR.
 */
public class VrDialog {
    private static final int DIALOG_WIDTH = 1200;
    private FrameLayout mVrLayout;
    private VrDialogManager mVrDialogManager;

    /**
     * The default constructor of VrDialog. Starts a new {@link Handler} on
     * UI Thread that will be used to send the events to the Dialog.
     */
    public VrDialog(VrDialogManager vrDialogManager) {
        mVrDialogManager = vrDialogManager;
    }

    /**
     * @return The dialog that is set to be shown in VR.
     */
    public FrameLayout getLayout() {
        return mVrLayout;
    }

    /**
     * Set the layout to be shown in VR.
     */
    public void setLayout(FrameLayout vrLayout) {
        mVrLayout = vrLayout;
        vrLayout.setLayoutParams(
                new FrameLayout.LayoutParams(DIALOG_WIDTH, ViewGroup.LayoutParams.WRAP_CONTENT));
    }

    /**
     * Dismiss whatever dialog that is shown in VR.
     */
    public void dismiss() {
        mVrDialogManager.closeVrDialog();
    }

    /**
     * Initialize a dialog in VR based on the layout that was set by {@link
     * #setLayout(FrameLayout)}. This also adds a OnLayoutChangeListener to make sure that Dialog in
     * VR has the correct size.
     */
    public void initVrDialog() {
        mVrLayout.addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                mVrDialogManager.setDialogSize(mVrLayout.getWidth(), mVrLayout.getHeight());
            }
        });
        disbaleSoftKeyboard(mVrLayout);
        mVrDialogManager.initVrDialog(mVrLayout.getWidth(), mVrLayout.getHeight());
    }

    private void disbaleSoftKeyboard(ViewGroup viewGroup) {
        for (int i = 0; i < viewGroup.getChildCount(); i++) {
            View view = viewGroup.getChildAt(i);
            if (view instanceof ViewGroup) {
                disbaleSoftKeyboard((ViewGroup) view);
            } else if (view instanceof TextView) {
                ((TextView) view).setInputType(InputType.TYPE_NULL);
            }
        }
    }
}
