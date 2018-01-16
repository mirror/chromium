// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import android.os.ResultReceiver;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

import org.chromium.base.VisibleForTesting;
import org.chromium.content.browser.input.ImeAdapterImpl;
import org.chromium.content.browser.input.InputMethodManagerWrapper;

/**
 * Adapts and plumbs android IME service onto the chrome text input API.
 * ImeAdapter provides an interface in both ways native <-> java:
 * 1. InputConnectionAdapter notifies native code of text composition state and
 *    dispatch key events from java -> WebKit.
 * 2. Native ImeAdapter notifies java side to clear composition text.
 *
 * The basic flow is:
 * 1. When InputConnectionAdapter gets called with composition or result text:
 *    If we receive a composition text or a result text, then we just need to
 *    dispatch a synthetic key event with special keycode 229, and then dispatch
 *    the composition or result text.
 * 2. Intercept dispatchKeyEvent() method for key events not handled by IME, we
 *   need to dispatch them to webkit and check webkit's reply. Then inject a
 *   new key event for further processing if webkit didn't handle it.
 *
 * Note that the native peer object does not take any strong reference onto the
 * instance of this java object, hence it is up to the client of this class (e.g.
 * the ViewEmbedder implementor) to hold a strong reference to it for the required
 * lifetime of the object.
 */
public interface ImeAdapter {

    static final int COMPOSITION_KEY_CODE = 229;

    /**
     * @param webContents {@link WebContents} object.
     * @return {@link ImeAdapter} object used for the give WebContents.
     *         {@code null} if not available.
     */
    static ImeAdapter fromWebContents(WebContents webContents) {
        return ImeAdapterImpl.fromWebContents(webContents);
    }

    /**
     * Add {@link ImeEventObserver} object to {@link ImeAdapter}.
     * @param observer imeEventObserver instance to add.
     */
    void addEventObserver(ImeEventObserver observer);

    /**
     * @see View#onCreateInputConnection(EditorInfo)
     */
    InputConnection onCreateInputConnection(EditorInfo outAttrs);

    /**
     * @see View#onCheckIsTextEditor()
     */
    boolean onCheckIsTextEditor();

    @VisibleForTesting
    ResultReceiver getNewShowKeyboardReceiver();

    /**
     * Get the current input connection for testing purposes.
     */
    @VisibleForTesting
    InputConnection getInputConnectionForTest();

    /**
     * Overrides the InputMethodManagerWrapper that ImeAdapter uses to make calls to
     * InputMethodManager.
     * @param immw InputMethodManagerWrapper that should be used to call InputMethodManager.
     */
    @VisibleForTesting
    void setInputMethodManagerWrapperForTest(InputMethodManagerWrapper immw);
}
