// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.view.View;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.ChromeAlertDialog;

@JNINamespace("vr_shell")
public class VrDialog extends VrView {
    private static final String TAG = "VrDialog";

    protected View mView;

    public VrDialog(long vrShellDelegate) {
        super(vrShellDelegate);
    }

    @Override
    public boolean show() {
        super.show();
        // TODO validate the view
        setLayout(new LinearLayout(mView.getContext()));
        getLayout().setOrientation(LinearLayout.VERTICAL);
        LayoutParams params = new LinearLayout.LayoutParams(1200, 800);

        getLayout().addView(mView, params);
        VrShellDelegate.setDialogView(getLayout(), "");
        initVrView();
        return true;
    }

    @Override
    public void setView(View view) {
        mView = view;
    }

    @Override
    public void setParent(ChromeAlertDialog dialog) {}
}
