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
import org.chromium.base.annotations.CalledByNative;

import java.util.List;

public class VrAlertDialog implements DialogRenderer {
    private static final String TAG = "DialogRenderer";
    private long mNativeVrShellDelegate;
    protected View mView;

    public VrAlertDialog(long vrShellDelegate) {
        Log.e(TAG, "VRP: constructor");
        mNativeVrShellDelegate = vrShellDelegate;
    }

    @Override
    public void show() {
        Log.e(TAG, "VRP: show");
        String textMessage = "";
        if (mView instanceof ViewGroup)
            textMessage = loopViews((ViewGroup) mView);
        else
            Log.e("PermissionDialogController", "not a group");

        nativeInitVrAlert(mNativeVrShellDelegate, textMessage);
    }

    @Override
    public void setView(View view) {
        mView = view;
    }

    private int resolveDialogTheme(Context context, int resid) {
        if (resid >= 0x01000000) { // start of real resource IDs.
            return resid;
        } else {
            TypedValue outValue = new TypedValue();
            context.getTheme().resolveAttribute(R.attr.alertDialogTheme, outValue, true);
            return outValue.resourceId;
        }
    }

    @Override
    public void setButton(
            int whichButton, CharSequence text, DialogInterface.OnClickListener listener) {
        // Set callback for the native class
        // nativeSetButton(whichButton, text);
    }

    private String loopViews(ViewGroup view) {
        String result = "";
        for (int i = 0; i < view.getChildCount(); i++) {
            View v = view.getChildAt(i);
            if (v instanceof TextView) {
                // Do something
                result += ((TextView) v).getText();
                result += " ";
                Log.e(TAG, "VRP: " + ((TextView) v).getText());
            } else if (v instanceof ViewGroup) {
                // result += this.loopViews((ViewGroup) v);
            }
        }
        return result;
    }

    @CalledByNative
    public void OnButtonClicked(/*int  whichButton*/) {
        // switch (whichButton) callback
        Log.e(TAG, "VRP: Clicked");
    }

    private native long nativeInitVrAlert(long vrShellDelegate, String textMessage);
}
