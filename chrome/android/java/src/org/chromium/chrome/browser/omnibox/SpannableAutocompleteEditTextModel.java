// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import android.text.Editable;
import android.text.Selection;
import android.text.SpannableString;
import android.text.Spanned;
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
    private boolean mLastEditWasPaste;
    private boolean mLastEditWasDelete;
    private boolean mIgnoreTextChangeFromAutocomplete = true;

    private final AutocompleteState mCurrentState;
    private final AutocompleteState mPreviouslyNotifiedState;
    private AutocompleteState mSetAutocompleteState;
    private final AutocompleteState mPreBatchEditState;

    private int mBatchEditNestCount;

    // For testing.
    private int mLastUpdateSelStart;
    private int mLastUpdateSelEnd;

    public SpannableAutocompleteEditTextModel(AutocompleteEditTextModelBase.Delegate delegate) {
        if (DEBUG) Log.i(TAG, "constructor");
        mDelegate = delegate;
        mCurrentState = new AutocompleteState(delegate.getText().toString(), "",
                delegate.getSelectionStart(), delegate.getSelectionEnd());
        mPreviouslyNotifiedState = new AutocompleteState(mCurrentState);
        mSetAutocompleteState = new AutocompleteState(mCurrentState);
        mPreBatchEditState = new AutocompleteState(mCurrentState);
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
        if (DEBUG) {
            Log.i(TAG, "setAutocompleteSpan. %s->%s", mSetAutocompleteState, mCurrentState);
        }
        if (!mCurrentState.isCursorAtEndOfUserText()
                || !mCurrentState.shiftAutocompleteTextFrom(mSetAutocompleteState)) {
            return;
        }
        mSetAutocompleteState.copyFrom(mCurrentState);

        int sel = mCurrentState.getSelStart();

        if (mSpan == null) mSpan = new BackgroundColorSpan(mDelegate.getHighlightColor());
        SpannableString spanString = new SpannableString(mCurrentState.mAutocompleteText);
        spanString.setSpan(
                mSpan, 0, mCurrentState.mAutocompleteText.length(), Spanned.SPAN_INTERMEDIATE);
        Editable editable = mDelegate.getEditableText();
        editable.append(spanString);

        // Keep the original selection before adding spannable string.
        Selection.setSelection(editable, sel, sel);
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
        boolean updateDisplay =
                !mPreviouslyNotifiedState.getUserText().equals(mCurrentState.getUserText());
        // Typing immediately after selecting the whole text (e.g. at the beginning) is not
        // considered deletion.
        boolean textDeleted = !mPreviouslyNotifiedState.isWholeTextSelected()
                && mCurrentState.isBackwardDeletedFrom(mPreviouslyNotifiedState);
        notifyAutocompleteTextStateChangedNoCheck(textDeleted, updateDisplay);
    }

    private void notifyAutocompleteTextStateChangedNoCheck(
            boolean textDeleted, boolean updateDisplay) {
        if (mIgnoreTextChangeFromAutocomplete) return;
        if (DEBUG) {
            Log.i(TAG, "notifyAutocompleteTextStateChanged PRV[%s] TXT[%s] DEL[%b] UPD[%b]",
                    mPreviouslyNotifiedState.getUserText(), mCurrentState.getUserText(),
                    textDeleted, updateDisplay);
        }
        mPreviouslyNotifiedState.copyFrom(mCurrentState);
        if (!textDeleted && !updateDisplay) return;
        mLastEditWasDelete = textDeleted;
        if (textDeleted) removeAutocomplete();
        mDelegate.onAutocompleteTextStateChanged(updateDisplay);
    }

    private void removeAutocomplete() {
        if (mInputConnection == null) return;
        if (DEBUG) Log.i(TAG, "removeAutocomplete");
        mCurrentState.setAutocompleteText("");
        mSetAutocompleteState.copyFrom(mCurrentState);
        mInputConnection.beginBatchEdit();
        removeAutocompleteSpan();
        mInputConnection.endBatchEdit();
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
        if (hasAutocomplete()) {
            mCurrentState.commitAutocomplete();
            mDelegate.getText().removeSpan(mSpan);
        }
        notifyAutocompleteTextStateChangedNoCheck(false, true);
    }

    @Override
    public void onSetText(CharSequence text) {
        if (DEBUG) Log.i(TAG, "onSetText: " + text);
    }

    @Override
    public void onSelectionChanged(int selStart, int selEnd) {
        if (DEBUG) Log.i(TAG, "onSelectionChanged [%d,%d]", selStart, selEnd);
        mCurrentState.setSelection(selStart, selEnd);
        if (mBatchEditNestCount > 0) return;
        int len = mCurrentState.getUserText().length();
        if (!mCurrentState.hasAutocompleteText()) {
            notifyAutocompleteTextStateChanged();
        } else if (selStart > len || selEnd > len) {
            if (DEBUG) Log.i(TAG, "Autocomplete text is being touched. Make it real.");
            commitAutocomplete();
        } else {
            if (DEBUG) Log.i(TAG, "Touching before the cursor removes autocomplete.");
            // Eventually calls removeAutocomplete().
            notifyAutocompleteTextStateChangedNoCheck(true, false);
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
        mLastEditWasPaste = false;
        if (Editable.class.isInstance(text)) {
            Editable editable = (Editable) text;
            int idx = getAutocompleteIndex(editable);
            if (idx != -1) {
                mCurrentState.setUserText(editable.subSequence(0, idx).toString());
                return;
            }
        }
        mCurrentState.setUserText(text.toString());
    }

    @Override
    public void onPaste() {
        if (DEBUG) Log.i(TAG, "onPaste");
        mLastEditWasPaste = true;
    }

    @Override
    public boolean wasLastEditPaste() {
        return mLastEditWasPaste;
    }

    @Override
    public String getTextWithAutocomplete() {
        String retVal = mCurrentState.getText();
        if (DEBUG) Log.i(TAG, "getTextWithAutocomplete: %s", retVal);
        return retVal;
    }

    @Override
    public String getTextWithoutAutocomplete() {
        String retVal = mCurrentState.getUserText();
        if (DEBUG) Log.i(TAG, "getTextWithoutAutocomplete: " + retVal);
        return retVal;
    }

    @Override
    public String getAutocompleteText() {
        return mCurrentState.getAutocompleteText();
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
        mSetAutocompleteState = new AutocompleteState(userText.toString(),
                inlineAutocompleteText.toString(), userText.length(), userText.length());
        if (mCurrentState.shiftAutocompleteTextFrom(mSetAutocompleteState)) {
            mSetAutocompleteState.copyFrom(mCurrentState);
        }
        // TODO(changwan): skip this if possible.
        if (mInputConnection != null) {
            mInputConnection.beginBatchEdit();
            mInputConnection.endBatchEdit();
        }
    }

    @Override
    public boolean shouldAutocomplete() {
        boolean retVal = mBatchEditNestCount == 0 && !mLastEditWasDelete && !wasLastEditPaste()
                && mCurrentState.isCursorAtEndOfUserText()
                && (!isComposing()
                           || isPhysicalKeyboardOneToOneTypable(getTextWithoutAutocomplete()));
        if (DEBUG) Log.i(TAG, "shouldAutocomplete: " + retVal);
        return retVal;
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
        // Cyrillic.
        return sAsciiEncoder.canEncode(text) && sAlphaNumericPunctPattern.matcher(text).matches();
    }

    @Override
    public boolean hasAutocomplete() {
        boolean retVal = mCurrentState.hasAutocompleteText();
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
                mPreBatchEditState.copyFrom(mCurrentState);
            }
            return retVal;
        }

        private void restoreBackspacedText(String diff) {
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
            if (DEBUG) Log.i(TAG, "endBatchEdit");

            // Note: this should be called before super.endBatchEdit() to be effective.
            String diff = mCurrentState.getBackwardDeletedTextFrom(mPreBatchEditState);
            if (diff != null && mPreBatchEditState.hasAutocompleteText()) {
                // Undo delete to retain the last character while removing autocomplete text.
                // Update selection first such that keyboard app gets what it expects.
                super.endBatchEdit();

                restoreBackspacedText(diff);
                // Text did not change but still notify the deletion.
                notifyAutocompleteTextStateChangedNoCheck(true, false);
                return true;
            }
            setAutocompleteSpan();
            notifyAutocompleteTextStateChanged();
            return super.endBatchEdit();
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