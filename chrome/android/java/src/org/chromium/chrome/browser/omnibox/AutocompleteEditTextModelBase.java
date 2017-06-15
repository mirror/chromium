// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import android.text.Editable;
import android.view.KeyEvent;
import android.view.inputmethod.InputConnection;

import org.chromium.base.VisibleForTesting;

import java.util.concurrent.Callable;

/**
 * An abstraction of the text model to show, keep track of, and update autocomplete.
 */
public interface AutocompleteEditTextModelBase {
    /**
     * An embedder should implement this.
     */
    public interface Delegate {
        // View methods
        Editable getText();
        Editable getEditableText();
        void append(CharSequence subSequence);
        int getSelectionStart();
        int getSelectionEnd();
        void setSelection(int autocompleteIndex, int length);
        void announceForAccessibility(CharSequence inlineAutocompleteText);
        int getHighlightColor();

        /**
         * This is called when autocomplete replaces the whole text.
         * @param text The text.
         */
        void replaceAllTextFromAutocomplete(String text);

        /**
         * This is called when there is a typing accessibility event that actually causes no change.
         * @param selectionStart The selection start.
         */
        void onNoChangeTypingAccessibilityEvent(int selectionStart);

        /**
         * This is called when autocomplete text state changes.
         * @param textDeleted True if text is just deleted.
         * @param updateDisplay True if string is changed.
         */
        void onAutocompleteTextStateChanged(boolean textDeleted, boolean updateDisplay);

        /**
         * This is called roughly the same time as when we call
         * InputMethodManager#updateSelection().
         *
         * @param selStart Selection start.
         * @param selEnd Selection end.
         */
        void onUpdateSelectionForTesting(int selStart, int selEnd);
    }

    InputConnection onCreateInputConnection(InputConnection inputConnection);
    boolean dispatchKeyEvent(KeyEvent event, Callable<Boolean> superDispatchKeyEvent);
    void onSetText(CharSequence text);
    void onSelectionChanged(int selStart, int selEnd);
    void onFocusChanged(boolean focused);
    void onTextChanged(CharSequence text, int start, int beforeLength, int afterLength);
    void onPaste();

    /**
     * @return Whether or not the user just pasted text.
     */
    boolean isPastedText();

    /**
     * @return The whole text including both user text and autocomplete text.
     */
    String getTextWithAutocomplete();

    /**
     * @return The user text without the autocomplete text.
     */
    String getTextWithoutAutocomplete();

    /**
     * Returns the length of the autocomplete text currently displayed, zero if none is
     * currently displayed.
     */
    String getAutocompleteText();

    /**
     * Sets whether text changes should trigger autocomplete.
     * @param ignore Whether text changes should be ignored and no auto complete.
     */
    void setIgnoreTextChangeFromAutocomplete(boolean ignore);

    /** @return Whether we should ignore text change from autocomplete. */
    boolean shouldIgnoreTextChangeFromAutocomplete();

    /**
     * Autocompletes the text and selects the text that was not entered by the user. Using append()
     * instead of setText() to preserve the soft-keyboard layout.
     * @param userText user The text entered by the user.
     * @param inlineAutocompleteText The suggested autocompletion for the user's text.
     */
    void setAutocompleteText(CharSequence userText, CharSequence inlineAutocompleteText);

    /**
     * Whether we want to be showing inline autocomplete results. We don't want to show them as the
     * user deletes input. Also if there is a composition (e.g. while using the Japanese IME),
     * we must not autocomplete or we'll destroy the composition.
     * @return Whether we want to be showing inline autocomplete results.
     */
    boolean shouldAutocomplete();

    /** @return Whether any autocomplete information is specified on the current text. */
    @VisibleForTesting
    boolean hasAutocomplete();

    /** @return The current {@link InputConnection} object. */
    @VisibleForTesting
    InputConnection getInputConnection();

    /** Run this when we update selection. */
    @VisibleForTesting
    void updateSelectionForTesting();
}
