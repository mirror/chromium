// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.keyboard;

import android.os.Handler;
import android.view.View;
import android.view.inputmethod.EditorInfo;

import org.chromium.base.Log;
import org.chromium.content_public.browser.ChromiumBaseInputConnection;
import org.chromium.content_public.browser.ImeAdapter;

/**
 * A factory class for {@link VrInputConnection}.
 */
public class VrInputConnectionFactory implements ChromiumBaseInputConnection.Factory {
    private static final String TAG = "cr_VrImeFact";
    private static final boolean DEBUG_LOGS = false;

    private VrInputConnection mInputConnection;

    /**
     * The interface used to receive updates from the Daydream keyboard.
     */
    public interface KeyboardClient {
        /**
         * Called when an edit is made using the Daydream keyboard.
         */
        void onTextInputEdited(String text, int selectionStart, int selectionEnd,
                int compositionStart, int compositionEnd);

        /**
         * Called when an commit is made using the Daydream keyboard.
         */
        void onTextInputCommitted(String text, int selectionStart, int selectionEnd,
                int compositionStart, int compositionEnd);
    }

    @Override
    public VrInputConnection initializeAndGet(View view, ImeAdapter imeAdapter, int inputType,
            int inputFlags, int inputMode, int selectionStart, int selectionEnd,
            EditorInfo outAttrs) {
        if (DEBUG_LOGS) Log.i(TAG, "initializeAndGet");
        if (mInputConnection == null) {
            mInputConnection = new VrInputConnection(view, imeAdapter);
        }
        return mInputConnection;
    }

    @Override
    public void onWindowFocusChanged(boolean gainFocus) {}

    @Override
    public void onViewFocusChanged(boolean gainFocus) {}

    @Override
    public void onViewAttachedToWindow() {}

    @Override
    public void onViewDetachedFromWindow() {}

    @Override
    public Handler getHandler() {
        assert false;
        return null;
    }

    /**
     * @return The active {@link KeyboardClient} if it exists.
     */
    public KeyboardClient getKeyboardClient() {
        return mInputConnection;
    }
}
