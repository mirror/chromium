// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.os.Handler;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.BaseInputConnection;

import org.chromium.base.Log;
import org.chromium.content.browser.input.ChromiumBaseInputConnection;
import org.chromium.content.browser.input.ImeAdapter;


public class VrInputConnection extends BaseInputConnection implements ChromiumBaseInputConnection {
    private static final String TAG = "cr_VrConn";
    private static final boolean DEBUG_LOGS = true;

    private ImeAdapter mImeAdapter;

    VrInputConnection(View view, ImeAdapter adapter) {
        super(view, true);
        mImeAdapter = adapter;
    }

    @Override
    public boolean commitText(final CharSequence text, final int newCursorPosition) {
        if (DEBUG_LOGS) Log.i(TAG, "commitText [%s] [%d]", text, newCursorPosition);
        mImeAdapter.sendCompositionToNative(text, newCursorPosition, true, 0);
        return true;
    }

    @Override
    public void updateStateOnUiThread(String text, int selectionStart, int selectionEnd,
            int compositionStart, int compositionEnd, boolean singleLine, boolean replyToRequest) {}

    /**
     * Send key event on UI thread.
     * @param event A key event.
     */
    @Override
    public boolean sendKeyEventOnUiThread(KeyEvent event) {
        return false;
    }

    /**
     * Call this when restartInput() is called.
     */
    @Override
    public void onRestartInputOnUiThread() {}

    /**
     * Unblock thread function if needed, e.g. we found that we will
     * never get state update.
     */
    @Override
    public void unblockOnUiThread() {}
}
