// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.keyboard;

import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;

import org.chromium.base.Log;
import org.chromium.content_public.browser.ChromiumBaseInputConnection;
import org.chromium.content_public.browser.ImeAdapter;
import org.chromium.content_public.browser.Range;
import org.chromium.content_public.browser.TextInputState;

/**
 * An implementation of {@link InputConnection} to communicate updates from the Daydream Keyboard
 * to the ImeAdapter which ultimately get sent to the Renderer.
 */
public class VrInputConnection extends BaseInputConnection
        implements ChromiumBaseInputConnection, VrInputConnectionFactory.KeyboardClient {
    private static final String TAG = "cr_VrImeConn";
    private static final boolean DEBUG_LOGS = false;
    // @see EditorInfo.html#IME_ACTION_GO
    private static final int IME_ACTION_GO = 2;
    private static final TextInputState DELETE_ACK =
            new TextInputState("", new Range(0, 0), new Range(-1, -1), false, false);

    private ImeAdapter mImeAdapter;
    private TextInputState mCachedUpdateState;
    private boolean mWaitingForDeleteAck = false;

    VrInputConnection(View view, ImeAdapter adapter) {
        super(view, true);
        mImeAdapter = adapter;
    }

    @Override
    public boolean sendKeyEventOnUiThread(KeyEvent event) {
        return false;
    }

    @Override
    public void unblockOnUiThread() {}

    @Override
    public void updateStateOnUiThread(String text, int selectionStart, int selectionEnd,
            int compositionStart, int compositionEnd, boolean singleLine, boolean replyToRequest) {
        TextInputState state = new TextInputState(text, new Range(selectionStart, selectionEnd),
                new Range(compositionStart, compositionEnd), false, false);
        if (DEBUG_LOGS) Log.i(TAG, "updateStateOnUiThread: %s", state.toString());
        mCachedUpdateState = state;
        if (mWaitingForDeleteAck && state.equals(DELETE_ACK)) {
            mWaitingForDeleteAck = false;
            return;
        }
        mImeAdapter.updateExtractedText(
                0, ChromiumBaseInputConnection.convertToExtractedText(state));
        mImeAdapter.updateSelection(selectionStart, selectionEnd, compositionStart, compositionEnd);
    }

    @Override
    public void onTextInputEdited(String text, int selectionStart, int selectionEnd,
            int compositionStart, int compositionEnd) {
        if (compositionStart == compositionEnd) {
            compositionStart = -1;
            compositionEnd = -1;
        }
        TextInputState state = new TextInputState(text, new Range(selectionStart, selectionEnd),
                new Range(compositionStart, compositionEnd), false, false);
        if (DEBUG_LOGS) Log.i(TAG, "onTextInputEdited: %s", state.toString());
        // The Daydream keyboard api is different from the Android IME apis and we only get the "new
        // state" after a user action instead of commit or composition signals. So upon receiving
        // any update, we clear the text field, and set it with the new values.
        if (mCachedUpdateState != null) {
            int beforeLength = mCachedUpdateState.selection().start();
            int afterLength =
                    mCachedUpdateState.text().length() - mCachedUpdateState.selection().end();
            // TODO(ymalik): Clear selection text as well.
            mImeAdapter.deleteSurroundingText(beforeLength, afterLength);
            // Deleteting ultimately triggers a call to {@link updateStateOnUiThread} with the new
            // state which we don't want to set on the keyboard.
            if (!state.equals(DELETE_ACK)) {
                mWaitingForDeleteAck = true;
            }
        }
        mImeAdapter.sendCompositionToNative(state.text().toString(), 1, 0);
        mImeAdapter.setEditableSelectionOffsets(state.selection().start(), state.selection().end());
        if (compositionStart != compositionEnd) {
            mImeAdapter.setComposingRegion(compositionStart, compositionEnd);
        }
    }

    @Override
    public void onTextInputCommitted(String text, int selectionStart, int selectionEnd,
            int compositionStart, int compositionEnd) {
        if (DEBUG_LOGS) Log.i(TAG, "onTextInputCommitted");
        mImeAdapter.performEditorAction(IME_ACTION_GO);
    }
}
