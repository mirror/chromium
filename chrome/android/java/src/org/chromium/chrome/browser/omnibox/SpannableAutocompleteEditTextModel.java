// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import android.text.Editable;
import android.text.Selection;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.BackgroundColorSpan;
import android.view.KeyEvent;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputConnectionWrapper;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;

import java.nio.charset.Charset;
import java.nio.charset.CharsetEncoder;
import java.util.Locale;
import java.util.concurrent.Callable;
import java.util.regex.Pattern;

// TODO(changwan): make this into a MVC model to keep the EditText's state internally, and stop
// querying them through getText() and other methods.
/**
 * An autocomplete model that appends autocomplete text at the end of query/URL text and selects it.
 */
public class SpannableAutocompleteEditTextModel implements AutocompleteEditTextModelBase {
    private static final String TAG = "cr_SpanAutocomplete";

    private static final boolean DEBUG = false;

    private static CharsetEncoder sAsciiEncoder = Charset.forName("US-ASCII").newEncoder();
    // A pattern that matches strings consisting only of alphabets, numbers, punctuations, and
    // a white space.
    private static Pattern sAlphaNumericPunctPattern = Pattern.compile("\\p{Print}*", 0);

    private AutocompleteEditTextModelBase.Delegate mDelegate;
    private AutocompleteInputConnection mInputConnection;
    private BackgroundColorSpan mSpan;
    private String mUserTextOnSetAutocomplete = "";
    private String mAutocompleteText = "";
    private boolean mIsPastedText;
    private boolean mLastEditWasDelete;
    private boolean mIgnoreTextChangeFromAutocomplete = true;
    private String mPreviouslyNotifiedText = "";
    private boolean mPreviouslyWholeTextSelected;

    private int mBatchEditNestCount;
    private String mPreBatchEditText;
    private boolean mPreBatchEditValidSelection;

    // For testing.
    private int mLastUpdateSelStart;
    private int mLastUpdateSelEnd;

    public SpannableAutocompleteEditTextModel(AutocompleteEditTextModelBase.Delegate delegate) {
        if (DEBUG) Log.i(TAG, "constructor");
        mDelegate = delegate;
    }

    @Override
    public InputConnection onCreateInputConnection(InputConnection inputConnection) {
        removeAutocomplete();
        mLastUpdateSelStart = mDelegate.getSelectionStart();
        mLastUpdateSelEnd = mDelegate.getSelectionEnd();
        if (inputConnection == null) {
            if (DEBUG) Log.i(TAG, "onCreateInputConnection: null");
            mInputConnection = null;
            return null;
        }
        if (DEBUG) Log.i(TAG, "onCreateInputConnection");
        mInputConnection = new AutocompleteInputConnection();
        mInputConnection.setTarget(inputConnection);
        return mInputConnection;
    }

    private int getAutocompleteIndex(Editable editable) {
        if (editable == null || mSpan == null) return -1;
        return editable.getSpanStart(mSpan); // returns -1 if mSpan is not attached
    }

    private boolean removeAutocompleteSpan() {
        Editable editable = mDelegate.getEditableText();
        int idx = getAutocompleteIndex(editable);
        if (idx == -1) return false;
        if (DEBUG) Log.i(TAG, "removeAutocompleteSpan IDX[%d]", idx);
        assert mBatchEditNestCount > 0;
        editable.removeSpan(mSpan);
        editable.delete(idx, editable.length());
        mSpan = null;
        if (DEBUG) {
            Log.i(TAG, "removeAutocompleteSpan - after: " + getEditableDebugString(editable));
        }
        return true;
    }

    private void setAutocompleteSpan() {
        assert mBatchEditNestCount > 0;
        removeAutocompleteSpan();
        Editable editable = mDelegate.getEditableText();
        String userText = getTextWithoutAutocomplete();
        if (TextUtils.isEmpty(mAutocompleteText) || !isCursorAtEndOfUserText()) return;

        if (!userText.equals(mUserTextOnSetAutocomplete)) {
            int growth = userText.length() - mUserTextOnSetAutocomplete.length();
            // User text has grown but still match prefix of mAutocompleteText.
            // TODO(changwan): check if we need to care about surrogate pair matching.
            if (growth > 0
                    && (mUserTextOnSetAutocomplete + mAutocompleteText).startsWith(userText)) {
                mUserTextOnSetAutocomplete = userText;
                mAutocompleteText = mAutocompleteText.substring(growth);
            } else {
                if (DEBUG) Log.i(TAG, "setAutocompleteSpan - match broken");
                return;
            }
        }
        if (DEBUG) Log.i(TAG, "setAutocompleteSpan. %s[%s]", userText, mAutocompleteText);
        int selStart = Selection.getSelectionStart(editable);
        int selEnd = Selection.getSelectionEnd(editable);
        if (selStart != selEnd) {
            if (DEBUG) Log.i(TAG, "setAutocompleteSpan fails.");
            return;
        }
        mSpan = new BackgroundColorSpan(mDelegate.getHighlightColor());
        SpannableString spanString = new SpannableString(mAutocompleteText);
        spanString.setSpan(mSpan, 0, mAutocompleteText.length(), Spanned.SPAN_INTERMEDIATE);
        editable.append(spanString);
        // Keep the original selection.
        Selection.setSelection(editable, selStart, selEnd);
        if (DEBUG) Log.i(TAG, "setAutocompleteSpan - after: " + getEditableDebugString(editable));
    }

    /**
     * @param editable The editable.
     * @return Debug string for the given {@Editable}.
     */
    private static String getEditableDebugString(Editable editable) {
        return String.format(Locale.US, "Editable {[%s] SEL[%d %d] COM[%d %d]}",
                editable.toString(), Selection.getSelectionStart(editable),
                Selection.getSelectionEnd(editable),
                BaseInputConnection.getComposingSpanStart(editable),
                BaseInputConnection.getComposingSpanEnd(editable));
    }

    private void notifyAutocompleteTextStateChanged() {
        String newText = getTextWithoutAutocomplete();
        boolean updateDisplay = !mPreviouslyNotifiedText.equals(newText);
        // Typing immediately after selecting the whole text (e.g. at the beginning) is not
        // considered deletion.
        boolean textDeleted = !mPreviouslyWholeTextSelected
                && mPreviouslyNotifiedText.length() > newText.length();
        notifyAutocompleteTextStateChangedNoCheck(newText, textDeleted, updateDisplay);
    }

    private void notifyAutocompleteTextStateChangedNoCheck(
            String newText, boolean textDeleted, boolean updateDisplay) {
        if (mIgnoreTextChangeFromAutocomplete) return;
        if (DEBUG) {
            Log.i(TAG, "notifyAutocompleteTextStateChanged PRV[%s] TXT[%s] DEL[%b] UPD[%b]",
                    mPreviouslyNotifiedText, newText, textDeleted, updateDisplay);
        }
        mPreviouslyNotifiedText = newText;
        mPreviouslyWholeTextSelected = isWholeTextSelected();
        mLastEditWasDelete = textDeleted;
        if (textDeleted) removeAutocomplete();
        if (textDeleted || updateDisplay) mDelegate.onAutocompleteTextStateChanged(updateDisplay);
    }

    private void removeAutocomplete() {
        if (mInputConnection == null) return;
        mAutocompleteText = "";
        mInputConnection.beginBatchEdit();
        removeAutocompleteSpan();
        mInputConnection.endBatchEdit();
        mUserTextOnSetAutocomplete = mDelegate.getText().toString();
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event, Callable<Boolean> superDispatchKeyEvent) {
        if (DEBUG) Log.i(TAG, "dispatchKeyEvent");
        if (mInputConnection == null) {
            try {
                return superDispatchKeyEvent.call();
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }
        if (event.getKeyCode() == KeyEvent.KEYCODE_ENTER) {
            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                return mInputConnection.commitAutocompleteAndRunImeOperation(superDispatchKeyEvent);
            }
            try {
                return superDispatchKeyEvent.call();
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }
        return mInputConnection.runImeOperation(superDispatchKeyEvent);
    }

    private void commitAutocomplete() {
        if (DEBUG) Log.i(TAG, "commitAutocomplete");
        if (hasAutocomplete()) mDelegate.getText().removeSpan(mSpan);
        notifyAutocompleteTextStateChangedNoCheck(mDelegate.getText().toString(), false, true);
    }

    @Override
    public void onSetText(CharSequence text) {
        if (DEBUG) Log.i(TAG, "onSetText: " + text);
    }

    @Override
    public void onSelectionChanged(int selStart, int selEnd) {
        if (DEBUG) Log.i(TAG, "onSelectionChanged [%d,%d]", selStart, selEnd);
        if (mBatchEditNestCount > 0) return;
        int idx = getAutocompleteIndex(mDelegate.getText());
        if (idx == -1) {
            notifyAutocompleteTextStateChanged();
        } else if (selStart > idx || selEnd > idx) {
            // Autocomplete text is being touched. Make it real.
            commitAutocomplete();
        } else {
            // Touching the cursor or before the cursor removes autocomplete. Eventually calls
            // removeAutocomplete().
            notifyAutocompleteTextStateChangedNoCheck(mDelegate.getText().toString(), true, false);
        }
        updateSelectionForTesting();
    }

    @Override
    public void onFocusChanged(boolean focused) {
        if (DEBUG) Log.i(TAG, "onFocusChanged: " + focused);
    }

    @Override
    public void onTextChanged(CharSequence text, int start, int beforeLength, int afterLength) {
        if (DEBUG) Log.i(TAG, "onTextChanged: " + text);
        mIsPastedText = false;
        if (mBatchEditNestCount == 0) {
            // Somehow the text changed without any IME command.
            commitAutocomplete();
        }
    }

    @Override
    public void onPaste() {
        if (DEBUG) Log.i(TAG, "onPaste");
        mIsPastedText = true;
    }

    @Override
    public boolean isPastedText() {
        return mIsPastedText;
    }

    @Override
    public String getTextWithAutocomplete() {
        String retVal = mDelegate.getText().toString();
        if (DEBUG) Log.i(TAG, "getTextWithAutocomplete: %s", retVal);
        return retVal;
    }

    @Override
    public String getTextWithoutAutocomplete() {
        Editable editable = mDelegate.getEditableText();
        if (editable == null) return "";
        String retVal = null;
        int idx = getAutocompleteIndex(editable);
        if (idx == -1) {
            retVal = editable.toString();
        } else {
            retVal = editable.subSequence(0, idx).toString();
        }
        if (DEBUG) Log.i(TAG, "getTextWithoutAutocomplete: " + retVal);
        return retVal;
    }

    @Override
    public String getAutocompleteText() {
        return mAutocompleteText;
    }

    @Override
    public void setIgnoreTextChangeFromAutocomplete(boolean ignore) {
        if (DEBUG) Log.i(TAG, "setIgnoreText: " + ignore);
        mIgnoreTextChangeFromAutocomplete = ignore;
    }

    @Override
    public boolean shouldIgnoreTextChangeFromAutocomplete() {
        return mIgnoreTextChangeFromAutocomplete;
    }

    @Override
    public void setAutocompleteText(CharSequence userText, CharSequence inlineAutocompleteText) {
        if (DEBUG) Log.i(TAG, "setAutocompleteText: %s[%s]", userText, inlineAutocompleteText);
        mUserTextOnSetAutocomplete = userText.toString();
        mAutocompleteText = inlineAutocompleteText.toString();
        if (mInputConnection != null) {
            mInputConnection.beginBatchEdit();
            mInputConnection.endBatchEdit();
        }
    }

    @Override
    public boolean shouldAutocomplete() {
        boolean retVal = mBatchEditNestCount == 0 && !mLastEditWasDelete && !isPastedText()
                && isCursorAtEndOfUserText()
                && (!isComposing()
                           || isPhysicalKeyboardOneToOneTypable(getTextWithoutAutocomplete()));
        if (DEBUG) Log.i(TAG, "shouldAutocomplete: " + retVal);
        return retVal;
    }

    private boolean isCursorAtEndOfUserText() {
        Editable editable = mDelegate.getText();
        int selStart = mDelegate.getSelectionStart();
        int selEnd = mDelegate.getSelectionEnd();
        // Check if cursor is at the end of user text.
        int expectedSelStart = getAutocompleteIndex(editable);
        // Span does not exist.
        if (expectedSelStart == -1) expectedSelStart = editable.length();
        int expectedSelEnd = expectedSelStart;
        return selStart == expectedSelStart && selEnd == expectedSelEnd;
    }

    private boolean isComposing() {
        Editable text = mDelegate.getText();
        return BaseInputConnection.getComposingSpanEnd(text)
                != BaseInputConnection.getComposingSpanStart(text);
    }

    @VisibleForTesting
    public static boolean isPhysicalKeyboardOneToOneTypable(String text) {
        // To start with, we are only activating this for English language and URLs
        // to avoid potential bad interactions with more complex IMEs.
        // TODO(changwan): also scan for other traditionally non-IME charsets like Latin1 and
        // Cyrillic
        return sAsciiEncoder.canEncode(text) && sAlphaNumericPunctPattern.matcher(text).matches();
    }

    private boolean isWholeTextSelected() {
        int selStart = mDelegate.getSelectionStart();
        int selEnd = mDelegate.getSelectionEnd();
        return selStart == 0 && selEnd == mDelegate.getText().length();
    }

    @Override
    public boolean hasAutocomplete() {
        boolean retVal = getAutocompleteIndex(mDelegate.getEditableText()) >= 0;
        if (DEBUG) Log.i(TAG, "hasAutocomplete: " + retVal);
        return retVal;
    }

    @Override
    public InputConnection getInputConnection() {
        return mInputConnection;
    }

    private void updateSelectionForTesting() {
        int selStart = mDelegate.getSelectionStart();
        int selEnd = mDelegate.getSelectionEnd();
        if (selStart == mLastUpdateSelStart && selEnd == mLastUpdateSelEnd) return;

        mLastUpdateSelStart = selStart;
        mLastUpdateSelEnd = selEnd;
        mDelegate.onUpdateSelectionForTesting(selStart, selEnd);
    }

    private class BatchEditTrackedInputConnection extends InputConnectionWrapper {
        public BatchEditTrackedInputConnection(InputConnection target, boolean mutable) {
            super(target, mutable);
        }

        @Override
        public boolean beginBatchEdit() {
            ++mBatchEditNestCount;
            return super.beginBatchEdit();
        }

        @Override
        public boolean endBatchEdit() {
            --mBatchEditNestCount;
            boolean retVal = super.endBatchEdit();
            if (mBatchEditNestCount == 0) updateSelectionForTesting();
            return retVal;
        }
    }

    private class AutocompleteInputConnection extends BatchEditTrackedInputConnection {
        public AutocompleteInputConnection() {
            super(null, true);
        }

        private <T> T commitAutocompleteAndRunImeOperation(Callable<T> c) {
            beginBatchEdit();
            commitAutocomplete();
            T retVal;
            try {
                retVal = c.call();
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
            endBatchEdit();
            return retVal;
        }

        private <T> T runImeOperation(Callable<T> c) {
            beginBatchEdit();
            T retVal;
            try {
                retVal = c.call();
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
            endBatchEdit();
            return retVal;
        }

        @Override
        public boolean beginBatchEdit() {
            boolean retVal = super.beginBatchEdit();
            // Note: this should be called after super.beginBatchEdit() to be effective.
            if (mBatchEditNestCount == 1) {
                if (DEBUG) Log.i(TAG, "beginBatchEdit");
                removeAutocompleteSpan();
                mPreBatchEditText = getTextWithoutAutocomplete();
                mPreBatchEditValidSelection = isCursorAtEndOfUserText();
            }
            return retVal;
        }

        private boolean isBackspacedText(String currText) {
            return mPreBatchEditValidSelection && isCursorAtEndOfUserText()
                    && !TextUtils.isEmpty(mAutocompleteText)
                    && mPreBatchEditText.startsWith(currText)
                    && mPreBatchEditText.length() > currText.length();
        }

        private void restoreBackspacedText(String currText) {
            String diff = mPreBatchEditText.substring(currText.length());
            if (DEBUG) Log.i(TAG, "restoreBackspacedText. diff: " + diff);
            // Append the deleted text.
            super.beginBatchEdit(); // just to avoid notifyAutocompleteTextStateChanged.
            Editable editable = mDelegate.getEditableText();
            editable.append(diff);
            super.endBatchEdit();
        }

        @Override
        public boolean endBatchEdit() {
            if (mBatchEditNestCount > 1) {
                return super.endBatchEdit();
            }

            // Note: this should be called before super.endBatchEdit() to be effective.
            String currText = getTextWithoutAutocomplete();
            if (isBackspacedText(currText)) {
                // Update selection first such that keyboard app gets what it expects.
                super.endBatchEdit();

                restoreBackspacedText(currText);
                // Text did not change but still notify the deletion.
                notifyAutocompleteTextStateChangedNoCheck(mPreBatchEditText, true, false);
                return true;
            }

            if (DEBUG) Log.i(TAG, "endBatchEdit");
            setAutocompleteSpan();
            notifyAutocompleteTextStateChanged();
            boolean retVal = super.endBatchEdit();
            return retVal;
        }

        @Override
        public boolean commitText(final CharSequence text, final int newCursorPosition) {
            if (DEBUG) Log.i(TAG, "commitText: " + text);
            return runImeOperation(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return AutocompleteInputConnection.super.commitText(text, newCursorPosition);
                }
            });
        }

        @Override
        public boolean setComposingText(final CharSequence text, final int newCursorPosition) {
            if (DEBUG) Log.i(TAG, "setComposingText: " + text);
            return runImeOperation(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return AutocompleteInputConnection.super.setComposingText(
                            text, newCursorPosition);
                }
            });
        }

        @Override
        public boolean setComposingRegion(final int start, final int end) {
            if (DEBUG) Log.i(TAG, "setComposingRegion: [%d,%d]", start, end);
            return runImeOperation(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return AutocompleteInputConnection.super.setComposingRegion(start, end);
                }
            });
        }

        @Override
        public boolean finishComposingText() {
            if (DEBUG) Log.i(TAG, "finishComposingText");
            return runImeOperation(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return AutocompleteInputConnection.super.finishComposingText();
                }
            });
        }

        @Override
        public boolean deleteSurroundingText(final int beforeLength, final int afterLength) {
            if (DEBUG) Log.i(TAG, "deleteSurroundingText [%d,%d]", beforeLength, afterLength);
            return runImeOperation(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return AutocompleteInputConnection.super.deleteSurroundingText(
                            beforeLength, afterLength);
                }
            });
        }

        @Override
        public boolean setSelection(final int start, final int end) {
            if (DEBUG) Log.i(TAG, "setSelection [%d,%d]", start, end);
            return runImeOperation(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return AutocompleteInputConnection.super.setSelection(start, end);
                }
            });
        }

        @Override
        public boolean performEditorAction(final int editorAction) {
            if (DEBUG) Log.i(TAG, "performEditorAction: " + editorAction);
            beginBatchEdit();
            commitAutocomplete();
            boolean retVal = runImeOperation(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return AutocompleteInputConnection.super.performEditorAction(editorAction);
                }
            });
            endBatchEdit();
            return retVal;
        }

        @Override
        public boolean sendKeyEvent(final KeyEvent event) {
            if (DEBUG) Log.i(TAG, "sendKeyEvent: " + event.getKeyCode());
            return runImeOperation(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return AutocompleteInputConnection.super.sendKeyEvent(event);
                }
            });
        }
        @Override
        public ExtractedText getExtractedText(final ExtractedTextRequest request, final int flags) {
            if (DEBUG) Log.i(TAG, "getExtractedText");
            return runImeOperation(new Callable<ExtractedText>() {
                @Override
                public ExtractedText call() {
                    return AutocompleteInputConnection.super.getExtractedText(request, flags);
                }
            });
        }

        @Override
        public CharSequence getTextAfterCursor(final int n, final int flags) {
            if (DEBUG) Log.i(TAG, "getTextAfterCursor");
            return runImeOperation(new Callable<CharSequence>() {
                @Override
                public CharSequence call() {
                    return AutocompleteInputConnection.super.getTextAfterCursor(n, flags);
                }
            });
        }

        @Override
        public CharSequence getTextBeforeCursor(final int n, final int flags) {
            if (DEBUG) Log.i(TAG, "getTextBeforeCursor");
            return runImeOperation(new Callable<CharSequence>() {
                @Override
                public CharSequence call() {
                    return AutocompleteInputConnection.super.getTextBeforeCursor(n, flags);
                }
            });
        }

        @Override
        public CharSequence getSelectedText(final int flags) {
            if (DEBUG) Log.i(TAG, "getSelectedText");
            return runImeOperation(new Callable<CharSequence>() {
                @Override
                public CharSequence call() {
                    return AutocompleteInputConnection.super.getSelectedText(flags);
                }
            });
        }
    }
}