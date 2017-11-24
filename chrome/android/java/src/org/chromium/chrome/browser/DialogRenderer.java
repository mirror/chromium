// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.app.Dialog;
import android.content.DialogInterface;
import android.view.View;

public interface DialogRenderer {
    public boolean show();
    public void setView(View view);
    public void setButton(
            int whichButton, CharSequence text, DialogInterface.OnClickListener listener);
    public void setParent(Dialog dialog);
    public boolean isShowing();
    public void dismiss();
}
