// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.keyboard;

import android.os.Handler;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.BaseInputConnection;

import org.chromium.base.Log;
import org.chromium.content.browser.input.ChromiumBaseInputConnection;
import org.chromium.content.browser.input.ImeAdapter;
import org.chromium.content.browser.input.Range;
import org.chromium.content.browser.input.TextInputState;

public class VrInputConnection extends BaseInputConnection implements ChromiumBaseInputConnection {
    private static final String TAG = "cr_VrConn";
    private static final boolean DEBUG_LOGS = true;
    private static final TextInputState mDeleteAck =
            new TextInputState("", new Range(0, 0), new Range(-1, -1), false, false);

    private ImeAdapter mImeAdapter;
    private TextInputState mCachedUpdateState;
    private boolean mWaitingForDeleteAck = false;

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

    @Override
    public void updateStateOnUiThread(String text, int selectionStart, int selectionEnd,
            int compositionStart, int compositionEnd, boolean singleLine, boolean replyToRequest) {
        TextInputState state = new TextInputState(text, new Range(selectionStart, selectionEnd),
                new Range(compositionStart, compositionEnd), false, false);
        if (DEBUG_LOGS) Log.i(TAG, "updateStateOnUiThread: %s", state.toString());
        mCachedUpdateState = state;
        if (mWaitingForDeleteAck && state.equals(mDeleteAck)) {
            mWaitingForDeleteAck = false;
            return;
        }
        mImeAdapter.updateExtractedText(
                0, ChromiumBaseInputConnection.convertToExtractedText(state));
        mImeAdapter.updateSelection(selectionStart, selectionEnd, compositionStart, compositionEnd);
    }

    public void onTextInputEdited(String text, int selectionStart, int selectionEnd,
            int compositionStart, int compositionEnd) {
        if (compositionStart == compositionEnd) {
            compositionStart = -1;
            compositionEnd = -1;
        }
        TextInputState state = new TextInputState(text, new Range(selectionStart, selectionEnd),
                new Range(compositionStart, compositionEnd), false, false);
        if (DEBUG_LOGS) Log.i(TAG, "onTextInputEdited: %s", state.toString());
        if (mCachedUpdateState != null) {
            int beforeLength = mCachedUpdateState.selection().start();
            int afterLength =
                    mCachedUpdateState.text().length() - mCachedUpdateState.selection().end();
            // TODO(ymalik): Clear selection text as well.
            mImeAdapter.deleteSurroundingText(beforeLength, afterLength);
            if (!state.equals(mDeleteAck)) {
                mWaitingForDeleteAck = true;
            }
        }
        mImeAdapter.sendCompositionToNative(state.text().toString(), 1, true, 0);
        mImeAdapter.setEditableSelectionOffsets(state.selection().start(), state.selection().end());
        if (compositionStart != compositionEnd) {
            mImeAdapter.setComposingRegion(compositionStart, compositionEnd);
        }
    }
}
