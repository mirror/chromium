// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.os.Handler;
import android.view.View;
import android.view.inputmethod.EditorInfo;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.content.browser.input.ChromiumBaseInputConnection;
import org.chromium.content.browser.input.ImeAdapter;

public class VrInputConnectionFactory implements ChromiumBaseInputConnection.Factory {
    private static final String TAG = "cr_VrFactory";

    private VrInputConnection mInputConnection;

    @Override
    public VrInputConnection initializeAndGet(View view, ImeAdapter imeAdapter, int inputType,
            int inputFlags, int inputMode, int selectionStart, int selectionEnd,
            EditorInfo outAttrs) {
        Log.e(TAG, "lolk VrInputConnectionFactory initializeAndGet");
        if (mInputConnection == null) {
            mInputConnection = new VrInputConnection(view, imeAdapter);
        }
        return mInputConnection;
    }

    @Override
    public void onWindowFocusChanged(boolean gainFocus) {
        Log.e(TAG, "lolk VrInputConnectionFactory onWindowFocusChanged: " + gainFocus);
    }
    @Override
    public void onViewFocusChanged(boolean gainFocus) {
        Log.e(TAG, "lolk VrInputConnectionFactory onViewFocusChanged: " + gainFocus);
    }
    @Override
    public void onViewAttachedToWindow() {
        Log.e(TAG, "lolk VrInputConnectionFactory onViewAttachedToWindow");
    }
    @Override
    public void onViewDetachedFromWindow() {
        Log.e(TAG, "lolk VrInputConnectionFactory onViewDetachedFromWindow");
    }

    /**
     * @return The {@link Handler} used for this InputConnection.
     */
    @Override
    @VisibleForTesting
    public Handler getHandler() {
        return null;
    }
}


