// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.content.Context;
import android.view.View;
import android.support.v7.app.AlertDialog;

import android.content.DialogInterface;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import android.view.ContextThemeWrapper;
import android.util.TypedValue;
import android.view.ViewGroup;
import android.widget.TextView;

import java.util.List;

public interface DialogRenderer {
    public void show();
    public void setView(View view);
    public void setButton(
            int whichButton, CharSequence text, DialogInterface.OnClickListener listener);
}
