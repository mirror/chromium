// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.input;

import android.os.IBinder;
import android.os.ResultReceiver;
import android.view.View;
import android.view.inputmethod.CursorAnchorInfo;
import android.view.inputmethod.InputMethodManager;

public interface ChromiumInputMethodManager {
    /**
     * @see android.view.inputmethod.InputMethodManager#restartInput(View)
     */
    public void restartInput(View view);

    /**
     * @see android.view.inputmethod.InputMethodManager#showSoftInput(View, int, ResultReceiver)
     */
    public void showSoftInput(View view, int flags, ResultReceiver resultReceiver);

    /**
     * @see android.view.inputmethod.InputMethodManager#isActive(View)
     */
    public boolean isActive(View view);

    /**
     * @see android.view.inputmethod.InputMethodManager#hideSoftInputFromWindow(IBinder, int,
     * ResultReceiver)
     */
    public boolean hideSoftInputFromWindow(
            IBinder windowToken, int flags, ResultReceiver resultReceiver);

    /**
     * @see android.view.inputmethod.InputMethodManager#updateSelection(View, int, int, int, int)
     */
    public void updateSelection(
            View view, int selStart, int selEnd, int candidatesStart, int candidatesEnd);

    /**
     * @see android.view.inputmethod.InputMethodManager#updateCursorAnchorInfo(View,
     * CursorAnchorInfo)
     */
    public void updateCursorAnchorInfo(View view, CursorAnchorInfo cursorAnchorInfo);

    /**
     * @see android.view.inputmethod.InputMethodManager
     * #updateExtractedText(View,int, ExtractedText)
     */
    public void updateExtractedText(
            View view, int token, android.view.inputmethod.ExtractedText text);

    /**
     * Notify that a user took some action with the current input method. Without this call
     * an input method app may wait longer when the user switches methods within the app.
     */
    public void notifyUserAction();

    /**
     * Whether the manager is for a VR session.
     */
    public boolean isVr();
}
