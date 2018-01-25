// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.keyboard;

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
    private static final String TAG = "cr_VrIme";
    private static final boolean DEBUG_LOGS = true;

    private final Context mContext;
    private View mView;
    private InputConnection mConnection;
    private BrowserKeyboardInterface mKeyboard;

    public interface BrowserKeyboardInterface {
        /**
         * Show the virtual keyboard.
         */
        void showSoftInput();

        /**
         * Hide the virtual keyboard.
         */
        void hideSoftInput();

        /**
         * Update the selection indeces.
         */
        void updateSelection(int selectionStart, int selectionEnd);

        /**
         * Update the composition indeces.
         */
        void updateComposition(int selectionStart, int selectionEnd);

        /**
         * Update the text.
         */
        void updateText(String text);
    }

    public VrInputMethodManager(Context context, BrowserKeyboardInterface keyboard) {
        mContext = context;
        mKeyboard = keyboard;
    }

    @Override
    public void restartInput(View view) {
        if (DEBUG_LOGS) Log.i(TAG, "restartInput");
        EditorInfo outAttrs = new EditorInfo();
        mConnection = view.onCreateInputConnection(outAttrs);
    }

    @Override
    public void showSoftInput(View view, int flags, ResultReceiver resultReceiver) {
        if (DEBUG_LOGS) Log.i(TAG, "showSoftInput");
        mView = view;
        EditorInfo outAttrs = new EditorInfo();
        mConnection = view.onCreateInputConnection(outAttrs);
        mKeyboard.showSoftInput();
    }

    @Override
    public boolean isActive(View view) {
        return mView != null && mView == view;
    }

    @Override
    public boolean hideSoftInputFromWindow(
            IBinder windowToken, int flags, ResultReceiver resultReceiver) {
        if (DEBUG_LOGS) Log.i(TAG, "hideSoftInputFromWindow");
        mKeyboard.hideSoftInput();
        mView = null;
        return false;
    }

    @Override
    public void updateSelection(
            View view, int selStart, int selEnd, int candidatesStart, int candidatesEnd) {
        if (DEBUG_LOGS) {
            Log.i(TAG, "updateSelection: SEL [%d, %d], COM [%d, %d]", selStart, selEnd,
                    candidatesStart, candidatesEnd);
        }
        mKeyboard.updateSelection(selStart, selEnd);
        mKeyboard.updateComposition(candidatesStart, candidatesEnd);
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
        mKeyboard.updateText(text.text.toString());
    }

    @Override
    public void notifyUserAction() {
        if (DEBUG_LOGS) Log.i(TAG, "notifyUserAction");
    }

    @Override
    public boolean isVr() {
        return true;
    }

    /* package */ void commitText(String text, int newCursorPosition) {
        if (DEBUG_LOGS) Log.i(TAG, "commitText [%s][%d]", text, newCursorPosition);
        if (mConnection != null) {
            mConnection.commitText(text, newCursorPosition);
        }
    }
}
