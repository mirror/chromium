// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.app.Dialog;
import android.content.DialogInterface;
import android.view.View;

/**
 * Defines the interface between Chrome dialogs and VR handlers
 */
public interface DialogRenderer {
    /**
     * Shows the dialog
     */
    public boolean show();

    /**
     * Sets the main view of the dialog.
     */
    public void setView(View view);

    /**
     * Set additional buttons that are not included in the main view
     */
    public void setButton(
            int whichButton, CharSequence text, DialogInterface.OnClickListener listener);

    /**
     * Set the parent dialog. This will be used to close th dialog.
     */
    public void setParent(Dialog dialog);

    /**
     * Returns true if dialog is showing
     */
    public boolean isShowing();

    /**
     * Closes the dialog
     */
    public void dismiss();
}
