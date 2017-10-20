// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.os.IBinder;
import android.os.ResultReceiver;
import android.os.StrictMode;
import android.view.View;
import android.view.inputmethod.CursorAnchorInfo;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;

import org.chromium.content.browser.input.ChromiumInputMethodManager;
import org.chromium.base.Log;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * Wrapper around Android's InputMethodManager
 */
public class VrInputMethodManager implements ChromiumInputMethodManager {
    private static final boolean DEBUG_LOGS = true;
    private static final String TAG = "cr_VrIme";

    private final Context mContext;

    private InputConnection mConnection;

    public VrInputMethodManager(Context context) {
        if (DEBUG_LOGS) Log.i(TAG, "Constructor");
        mContext = context;
        VrShellDelegate.setInputMethodManager(this);
    }

    @Override
    public void restartInput(View view) {
        if (DEBUG_LOGS) Log.i(TAG, "restartInput");
        // Call View.onCreateInputConnection
        EditorInfo outAttrs = new EditorInfo();
        mConnection = view.onCreateInputConnection(outAttrs);
    }

    @Override
    public void showSoftInput(View view, int flags, ResultReceiver resultReceiver) {
        if (DEBUG_LOGS) Log.i(TAG, "showSoftInput");
        EditorInfo outAttrs = new EditorInfo();
        mConnection = view.onCreateInputConnection(outAttrs);
        // Call native showSoftInput
        VrShellDelegate.showSoftInput();
    }

    @Override
    public boolean isActive(View view) {
        if (DEBUG_LOGS) Log.i(TAG, "isActive: ");
        return false;
    }

    @Override
    public boolean hideSoftInputFromWindow(IBinder windowToken, int flags,
            ResultReceiver resultReceiver) {
        if (DEBUG_LOGS) Log.i(TAG, "hideSoftInputFromWindow");
        return false;
    }

    @Override
    public void updateSelection(View view, int selStart, int selEnd,
            int candidatesStart, int candidatesEnd) {
        if (DEBUG_LOGS) {
            Log.i(TAG, "updateSelection: SEL [%d, %d], COM [%d, %d]", selStart, selEnd,
                    candidatesStart, candidatesEnd);
        }
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    @Override
    public void updateCursorAnchorInfo(View view, CursorAnchorInfo cursorAnchorInfo) {
        if (DEBUG_LOGS) Log.i(TAG, "updateCursorAnchorInfo");
    }

    @Override
    public void updateExtractedText(
            View view, int token, android.view.inputmethod.ExtractedText text) {
        if (DEBUG_LOGS) Log.d(TAG, "updateExtractedText");
    }

    @Override
    public void notifyUserAction() {
        // On N and above, this is not needed.
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.M) return;
        if (DEBUG_LOGS) Log.i(TAG, "notifyUserAction");
    }

    @Override
    public boolean isVr() {
      return true;
    }

    /* package */ void commitText(String text, int newCursorPosition) {
        if (DEBUG_LOGS) Log.i(TAG, "commitText: " + text);
        if (mConnection != null) {
          Log.i(TAG, "mConnection != null");
          mConnection.commitText(text, newCursorPosition);
        }
    }
}
