// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.keyboard;

import android.os.Handler;
import android.view.View;
import android.view.inputmethod.EditorInfo;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.content.browser.input.ChromiumBaseInputConnection;
import org.chromium.content.browser.input.ImeAdapter;

public class VrInputConnectionFactory implements ChromiumBaseInputConnection.Factory {
    private static final String TAG = "cr_VrFactory";
    private static final boolean DEBUG_LOGS = true;

    private VrInputConnection mInputConnection;

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
    public void onWindowFocusChanged(boolean gainFocus) {
        if (DEBUG_LOGS) Log.d(TAG, "onWindowFocusChanged: " + gainFocus);
    }

    @Override
    public void onViewFocusChanged(boolean gainFocus) {
        if (DEBUG_LOGS) Log.d(TAG, "onViewFocusChanged: " + gainFocus);
    }

    @Override
    public void onViewAttachedToWindow() {
        if (DEBUG_LOGS) Log.d(TAG, "onViewAttachedToWindow");
    }

    @Override
    public void onViewDetachedFromWindow() {
        if (DEBUG_LOGS) Log.d(TAG, "onViewDetachedFromWindow");
    }

    /**
     * @return The {@link Handler} used for this InputConnection.
     */
    @Override
    @VisibleForTesting
    public Handler getHandler() {
        return null;
    }

    /**
     * @return The active {@link VrInputConnection} if it exists.
     */
    public VrInputConnection getInputConnection() {
        return mInputConnection;
    }
}
