// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.keyboard;

import android.content.Context;
import android.os.IBinder;
import android.os.ResultReceiver;
import android.view.View;
import android.view.inputmethod.CursorAnchorInfo;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

import org.chromium.base.Log;
import org.chromium.content_public.browser.ChromiumInputMethodManagerWrapper;

/**
 * A fake wrapper around Android's InputMethodManager that doesn't really talk to the
 * InputMethodManager and instead talks to the Daydream keyboard.
 */
public class VrInputMethodManagerWrapper implements ChromiumInputMethodManagerWrapper {
    private static final String TAG = "cr_VrIme";
    private static final boolean DEBUG_LOGS = true;

    private final Context mContext;
    private View mView;
    private InputConnection mConnection;
    private BrowserKeyboardInterface mKeyboard;

    /**
     * The interface used by the browser to talk to the the Daydream keyboard.
     */
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
        void updateComposition(int compositionStart, int compositionEnd);

        /**
         * Update the text.
         */
        void updateText(String text);
    }

    public VrInputMethodManagerWrapper(Context context, BrowserKeyboardInterface keyboard) {
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

    @Override
    public void updateCursorAnchorInfo(View view, CursorAnchorInfo cursorAnchorInfo) {}

    @Override
    public void updateExtractedText(
            View view, int token, android.view.inputmethod.ExtractedText text) {
        if (DEBUG_LOGS) Log.i(TAG, "updateExtractedText: [%s]", text.text.toString());
        mKeyboard.updateText(text.text.toString());
    }

    @Override
    public void notifyUserAction() {}
}
