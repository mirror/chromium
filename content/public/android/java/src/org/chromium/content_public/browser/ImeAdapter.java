// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import android.content.Context;
import android.os.ResultReceiver;
import android.view.KeyEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.InputConnection;

import org.chromium.base.VisibleForTesting;
import org.chromium.content.browser.input.ImeAdapterImpl;

/**
 * Adapts and plumbs android IME service onto the chrome text input API.
 */
public interface ImeAdapter {
    /** Composition key code sent when user either hit a key or hit a selection. */
    @VisibleForTesting
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
     * @return the default {@link ChromiumInputMethodManagerWrapper} that the ImeAdapter uses to
     * make calls to the InputMethodManager.
     */
    static ChromiumInputMethodManagerWrapper createDefaultInputMethodManagerWrapper(
            Context context) {
        return ImeAdapterImpl.createDefaultInputMethodManagerWrapper(context);
    }

    /**
     * @return the default {@link ChromiumBaseInputConnection.Factory} that the ImeAdapter uses to
     * interface with the IME.
     */
    static ChromiumBaseInputConnection.Factory createDefaultInputConnectionFactory(
            ChromiumInputMethodManagerWrapper wrapper) {
        return ImeAdapterImpl.createDefaultInputConnectionFactory(wrapper);
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

    /**
     * @return a newly instantiated {@link ResultReceiver} used to scroll to the editable
     *     node at the right timing.
     */
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
    void setInputMethodManagerWrapper(ChromiumInputMethodManagerWrapper immw);

    /**
     * Overrides the ChromiumBaseInputConnection.Factory that ImeAdapter uses create an
     * InputConnection with the IME.
     * @param factory ChromiumBaseInputConnection.Factory that should be used.
     */
    void setInputConnectionFactory(ChromiumBaseInputConnection.Factory factory);

    /**
     * Update the composition with the given text and cursor position.
     *
     * @param text                The text to update the composition with.
     * @param newCursorPosition   The new cursor position with this update.
     * @param unicodeFromKeyEvent The unicode character generated with this update.
     */
    boolean sendCompositionToNative(
            CharSequence text, int newCursorPosition, int unicodeFromKeyEvent);

    /**
     * Send a commit with the given text and cursor position to the native counterpart.
     *
     * @param text                The text to commit.
     * @param newCursorPosition   The new cursor position after the commit.
     */
    boolean sendCommitToNative(CharSequence text, int newCursorPosition);

    /**
     * Send a request to the native counterpart to delete a given range of characters.
     *
     * @param beforeLength Number of characters to extend the selection by before the existing
     *                     selection.
     * @param afterLength Number of characters to extend the selection by after the existing
     *                    selection.
     * @return Whether the native counterpart of ImeAdapter received the call.
     */
    boolean deleteSurroundingText(int beforeLength, int afterLength);

    /**
     * Send a request to the native counterpart to delete a given range of characters.
     *
     * @param beforeLength Number of code points to extend the selection by before the existing
     *                     selection.
     * @param afterLength Number of code points to extend the selection by after the existing
     *                    selection.
     * @return Whether the native counterpart of ImeAdapter received the call.
     */
    boolean deleteSurroundingTextInCodePoints(int beforeLength, int afterLength);

    /**
     * Send a request to the native counterpart to set the selection to given range.
     *
     * @param start Selection start index.
     * @param end Selection end index.
     * @return Whether the native counterpart of ImeAdapter received the call.
     */
    boolean setEditableSelectionOffsets(int start, int end);

    /**
     * Send a request to the native counterpart to set composing region to given indices.
     *
     * @param start The start of the composition.
     * @param end The end of the composition.
     * @return Whether the native counterpart of ImeAdapter received the call.
     */
    boolean setComposingRegion(int start, int end);

    /**
     * @see InputConnection#finishComposingText()
     */
    boolean finishComposingText();

    /**
     * Send a request to the native counterpart to perform an editor action.
     *
     * @param actionCode One of the action constants for EditorInfo.editorType.
     * @return Whether the native counterpart of ImeAdapter received the call.
     */
    boolean performEditorAction(int actionCode);

    /**
     * @see BaseInputConnection#performContextMenuAction(int)
     */
    boolean performContextMenuAction(int id);

    /**
     * Send a request to the native counterpart to give the latest text input state update.
     */
    boolean requestTextInputStateUpdate();

    /**
     * Notified when IME requested Chrome to change the cursor update mode.
     */
    boolean onRequestCursorUpdates(int cursorUpdateMode);

    /**
     * @see InputConnection#sendKeyEvent(android.view.KeyEvent)
     */
    boolean sendKeyEvent(KeyEvent event);

    /**
     * Send a synthetic key press event to the native counterpart.
     */
    void sendSyntheticKeyPress(int keyCode, int flags);

    /**
     * Update selection to input method manager.
     *
     * @see android.view.inputmethod.InputMethodManager#updateSelection(View, int, int, int, int)
     */
    void updateSelection(
            int selectionStart, int selectionEnd, int compositionStart, int compositionEnd);

    /**
     * Update extracted text to input method manager.
     *
     * @see android.view.inputmethod.InputMethodManager
     * #updateExtractedText(View,int, ExtractedText)
     */
    void updateExtractedText(int token, ExtractedText extractedText);

    /**
     * Notify input method manager that the user took some action with this input method.
     */
    void notifyUserAction();
}
