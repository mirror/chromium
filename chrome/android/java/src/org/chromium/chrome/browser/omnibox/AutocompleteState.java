// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import android.text.TextUtils;

import org.chromium.base.VisibleForTesting;

import java.util.Locale;

/**
 * A state to keep track of EditText and autocomplete.
 */
class AutocompleteState {
    private String mUserText;
    String mAutocompleteText;
    private int mSelStart;
    private int mSelEnd;

    public AutocompleteState(AutocompleteState a) {
        copyFrom(a);
    }

    public AutocompleteState(String userText, String autocompleteText, int selStart, int selEnd) {
        mUserText = userText;
        mAutocompleteText = autocompleteText;
        mSelStart = selStart;
        mSelEnd = selEnd;
    }

    /** Copy from another state. */
    public void copyFrom(AutocompleteState a) {
        mUserText = a.mUserText;
        mAutocompleteText = a.mAutocompleteText;
        mSelStart = a.mSelStart;
        mSelEnd = a.mSelEnd;
    }

    /** @return The user text. */
    public String getUserText() {
        return mUserText;
    }

    /** @return The autocomplete text. */
    public String getAutocompleteText() {
        return mAutocompleteText;
    }

    /** @return Whether autocomplete text is non-empty. */
    public boolean hasAutocompleteText() {
        return !TextUtils.isEmpty(mAutocompleteText);
    }

    /** @return The whole text including autocomplete text. */
    public String getText() {
        return mUserText + mAutocompleteText;
    }

    /** @return The selection start. */
    public int getSelStart() {
        return mSelStart;
    }

    /** @return The selection end. */
    public int getSelEnd() {
        return mSelEnd;
    }

    /**
     * Set selection.
     * @param selStart The selection start.
     * @param selEnd The selection end.
     */
    public void setSelection(int selStart, int selEnd) {
        mSelStart = selStart;
        mSelEnd = selEnd;
    }

    /**
     * Set user text.
     * @param userText The user text.
     */
    public void setUserText(String userText) {
        mUserText = userText;
    }

    /**
     * Set autocomplete text
     * @param autocompleteText The autocomplete text.
     */
    public void setAutocompleteText(String autocompleteText) {
        mAutocompleteText = autocompleteText;
    }

    /** @return Whether the cursor is at the end of user text. */
    public boolean isCursorAtEndOfUserText() {
        return mSelStart == mUserText.length() && mSelEnd == mUserText.length();
    }

    /** @return Whether the whole text is selected. */
    public boolean isWholeTextSelected() {
        return mSelStart == 0 && mSelEnd == mUserText.length();
    }

    /**
     * @param prevState The previous state to compare the current state with.
     * @return Whether the current state is backward-deleted from prevState.
     */
    public boolean isBackwardDeletedFrom(AutocompleteState prevState) {
        return isCursorAtEndOfUserText() && prevState.isCursorAtEndOfUserText()
                && isProperSubstringOf(mUserText, prevState.mUserText);
    }

    /**
     * @param prevState The previous state to compare the current state with.
     * @return The differential string that has been backward deleted.
     */
    public String getBackwardDeletedTextFrom(AutocompleteState prevState) {
        if (!isBackwardDeletedFrom(prevState)) return null;
        return prevState.mUserText.substring(mUserText.length());
    }

    @VisibleForTesting
    public static boolean isProperSubstringOf(String a, String b) {
        return b.startsWith(a) && b.length() > a.length();
    }

    /**
     * Try to shift autocomplete text based on the prevState. For example, a[bc] can be shifted
     * to ab[c] where usertext[autocompletetext] denotes a state.
     * @param prevState The previous text to shift from.
     * @return Whether the shifting was successful.
     */
    public boolean shiftAutocompleteTextFrom(AutocompleteState prevState) {
        mAutocompleteText = "";

        // Shift when user text has grown but still prefix of prevState's whole text.
        int diff = mUserText.length() - prevState.mUserText.length();
        if (diff < 0) return false;
        if (!prevState.getText().startsWith(mUserText)) return false;
        mAutocompleteText = prevState.mAutocompleteText.substring(diff);
        return true;
    }

    public void commitAutocomplete() {
        mUserText += mAutocompleteText;
        mAutocompleteText = "";
    }

    @Override
    public boolean equals(Object o) {
        if (!(o instanceof AutocompleteState)) return false;
        if (o == this) return true;
        AutocompleteState a = (AutocompleteState) o;
        return mUserText.equals(a.mUserText) && mAutocompleteText.equals(a.mAutocompleteText)
                && mSelStart == a.mSelStart && mSelEnd == a.mSelEnd;
    }

    @Override
    public int hashCode() {
        return mUserText.hashCode() * 2 + mAutocompleteText.hashCode() * 3 + mSelStart * 5
                + mSelEnd * 7;
    }

    @Override
    public String toString() {
        return String.format(Locale.US, "AutocompleteState {[%s][%s] [%d-%d]}", mUserText,
                mAutocompleteText, mSelStart, mSelEnd);
    }
}