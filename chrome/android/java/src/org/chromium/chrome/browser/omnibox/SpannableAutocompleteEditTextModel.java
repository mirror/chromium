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

import java.util.Locale;
import java.util.concurrent.Callable;
import java.util.regex.Pattern;

/**
 * An autocomplete model that appends autocomplete text at the end of query/URL text and selects it.
 */
public class SpannableAutocompleteEditTextModel implements AutocompleteEditTextModelBase {
    private static final String TAG = "cr_SpanAutocomplete";

    private static final boolean DEBUG = false;

    // A pattern that matches strings consisting of English and European character sets, numbers,
    // punctuations, and a white space.
    private static Pattern sNonCompositionalTextPattern = Pattern.compile(
            "[\\p{script=latin}\\p{script=cyrillic}\\p{script=greek}\\p{script=hebrew}\\p{Punct} "
            + "0-9]*");

    private AutocompleteEditTextModelBase.Delegate mDelegate;
    private AutocompleteInputConnection mInputConnection;
    private boolean mLastEditWasTyping = true;
    private boolean mIgnoreTextChangeFromAutocomplete = true;

    // The current state that reflects EditText view's current state through callbacks such as
    // onSelectionChanged() and onTextChanged(). Also, it keeps track of the autocomplete text shown
    // to the user.
    private final AutocompleteState mCurrentState;

    // This keeps track of the state in which previous notification was sent. It prevents redundant
    // or unnecessary notification.
    private final AutocompleteState mPreviouslyNotifiedState;

    // This keeps track of the autocompletetext in regards to the usertext it was set for. It gets
    // updated in the following cases:
    // 1) when setAutcompleteText() is called.
    // 2) when autocomplete text is no longer valid or deleted.
    // 3) when autocomplete text gets shifted to match the newly typed characters.
    private final AutocompleteState mSetAutocompleteState;

    private final SpanController mSpanController;

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

        mSpanController = new SpanController(delegate);
    }

    @Override
    public InputConnection onCreateInputConnection(InputConnection inputConnection) {
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
        if (DEBUG) {
            Log.i(TAG, "notifyAutocompleteTextStateChanged PRV[%s] CUR[%s] IGN[%b]",
                    mPreviouslyNotifiedState, mCurrentState, mIgnoreTextChangeFromAutocomplete);
        }
        if (mIgnoreTextChangeFromAutocomplete) return;
        if (mBatchEditNestCount > 0) return;
        if (mCurrentState.equals(mPreviouslyNotifiedState)) return;
        // Nothing has changed except that autocomplete text has been set or modified.
        if (mCurrentState.equalsExceptAutocompleteText(mPreviouslyNotifiedState)
                && mCurrentState.hasAutocompleteText()) {
            // Autocomplete text is set by the controller, we should not notify the controller with
            // the same information.
            mPreviouslyNotifiedState.copyFrom(mCurrentState);
            return;
        }
        mPreviouslyNotifiedState.copyFrom(mCurrentState);
        // The current model's mechanism always moves the cursor at the end of user text, so we
        // don't need to update the display.
        mDelegate.onAutocompleteTextStateChanged(false /* updateDisplay */);
    }

    private void clearAutocompleteText() {
        if (DEBUG) Log.i(TAG, "clearAutocomplete");
        setAutocompleteTextInternal(mCurrentState.getUserText(), "");
    }

    @Override
    public boolean dispatchKeyEvent(final KeyEvent event) {
        if (DEBUG) Log.i(TAG, "dispatchKeyEvent");
        if (mInputConnection == null) {
            return mDelegate.super_dispatchKeyEvent(event);
        }
        if (event.getKeyCode() == KeyEvent.KEYCODE_ENTER) {
            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                return mInputConnection.commitAutocompleteAndRunImeOperation(
                        new Callable<Boolean>() {
                            @Override
                            public Boolean call() {
                                return mDelegate.super_dispatchKeyEvent(event);
                            }
                        });
            }
            mDelegate.super_dispatchKeyEvent(event);
        }
        return mInputConnection.runImeOperation(new Callable<Boolean>() {
            @Override
            public Boolean call() {
                return mDelegate.super_dispatchKeyEvent(event);
            }
        });
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
            if (mInputConnection != null) mInputConnection.commitAutocomplete();
        } else {
            if (DEBUG) Log.i(TAG, "Touching before the cursor removes autocomplete.");
            clearAutocompleteText();
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
        mSpanController.reflectTextUpdateInState(mCurrentState, text);
        if (mBatchEditNestCount > 0) return; // let endBatchEdit() handles changes from IME.
        // An external change such as text paste occurred.
        mLastEditWasTyping = false;
        clearAutocompleteText();
    }

    @Override
    public void onPaste() {
        if (DEBUG) Log.i(TAG, "onPaste");
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
    public void setAutocompleteText(CharSequence userText, CharSequence inlineAutocompleteText) {
        setAutocompleteTextInternal(userText.toString(), inlineAutocompleteText.toString());
    }

    private void setAutocompleteTextInternal(String userText, String autocompleteText) {
        if (DEBUG) Log.i(TAG, "setAutocompleteText: %s[%s]", userText, autocompleteText);
        mSetAutocompleteState.set(userText, autocompleteText, userText.length(), userText.length());
        // TODO(changwan): avoid this when necessary.
        if (mInputConnection != null) {
            mInputConnection.beginBatchEdit();
            mInputConnection.endBatchEdit();
        }
    }

    @Override
    public boolean shouldAutocomplete() {
        boolean retVal = mBatchEditNestCount == 0 && mLastEditWasTyping
                && mCurrentState.isCursorAtEndOfUserText()
                && isNonCompositionalText(getTextWithoutAutocomplete());
        if (DEBUG) Log.i(TAG, "shouldAutocomplete: " + retVal);
        return retVal;
    }

    @VisibleForTesting
    public static boolean isNonCompositionalText(String text) {
        // To start with, we are only activating this for English language and URLs
        // to avoid potential bad interactions with more complex IMEs.
        // TODO(changwan): also scan for other traditionally non-IME charsets.
        return sNonCompositionalTextPattern.matcher(text).matches();
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

    private static class SpanController {
        private Delegate mDelegate;
        private BackgroundColorSpan mSpan;

        public SpanController(Delegate delegate) {
            mDelegate = delegate;
        }

        public void setSpan(AutocompleteState state) {
            int sel = state.getSelStart();

            if (mSpan == null) mSpan = new BackgroundColorSpan(mDelegate.getHighlightColor());
            SpannableString spanString = new SpannableString(state.getAutocompleteText());
            spanString.setSpan(
                    mSpan, 0, state.getAutocompleteText().length(), Spanned.SPAN_INTERMEDIATE);
            Editable editable = mDelegate.getEditableText();
            editable.append(spanString);

            // Keep the original selection before adding spannable string.
            Selection.setSelection(editable, sel, sel);
            if (DEBUG) Log.i(TAG, "setSpan: " + getEditableDebugString(editable));
        }

        private int getSpanIndex(Editable editable) {
            if (editable == null || mSpan == null) return -1;
            return editable.getSpanStart(mSpan); // returns -1 if mSpan is not attached
        }

        public boolean removeSpan() {
            Editable editable = mDelegate.getEditableText();
            int idx = getSpanIndex(editable);
            if (idx == -1) return false;
            if (DEBUG) Log.i(TAG, "removeSpan IDX[%d]", idx);
            editable.removeSpan(mSpan);
            editable.delete(idx, editable.length());
            mSpan = null;
            if (DEBUG) {
                Log.i(TAG, "removeSpan - after: " + getEditableDebugString(editable));
            }
            return true;
        }

        public void commitSpan() {
            mDelegate.getEditableText().removeSpan(mSpan);
        }

        public void reflectTextUpdateInState(AutocompleteState state, CharSequence text) {
            if (Editable.class.isInstance(text)) {
                Editable editable = (Editable) text;
                int idx = getSpanIndex(editable);
                if (idx != -1) {
                    state.setUserText(editable.subSequence(0, idx).toString());
                    return;
                }
            }
            state.setUserText(text.toString());
        }
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
        private final AutocompleteState mPreBatchEditState;

        public AutocompleteInputConnection() {
            super(null, true);
            mPreBatchEditState = new AutocompleteState(mCurrentState);
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

        public void commitAutocomplete() {
            if (DEBUG) Log.i(TAG, "commitAutocomplete");
            if (hasAutocomplete()) {
                mCurrentState.commitAutocompleteText();
                mSetAutocompleteState.copyFrom(mCurrentState);
                mLastEditWasTyping = false;
                super.beginBatchEdit(); // avoids additional notifyAutocompleteTextStateChanged()
                mSpanController.commitSpan();
                super.endBatchEdit();
                notifyAutocompleteTextStateChanged();
            }
        }

        @Override
        public boolean beginBatchEdit() {
            boolean retVal = super.beginBatchEdit();
            // Note: this should be called after super.beginBatchEdit() to be effective.
            if (mBatchEditNestCount == 1) {
                if (DEBUG) Log.i(TAG, "beginBatchEdit");
                mPreBatchEditState.copyFrom(mCurrentState);
                mSpanController.removeSpan();
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

        private boolean setAutocompleteSpan() {
            assert mBatchEditNestCount > 0;
            mSpanController.removeSpan();
            if (DEBUG) {
                Log.i(TAG, "setAutocompleteSpan. %s->%s", mSetAutocompleteState, mCurrentState);
            }
            if (!mCurrentState.isCursorAtEndOfUserText()) return false;

            if (mCurrentState.reuseAutocompleteTextIfPrefixExtension(mSetAutocompleteState)) {
                mSetAutocompleteState.copyFrom(mCurrentState);
                mSpanController.setSpan(mCurrentState);
                return true;
            } else {
                return false;
            }
        }

        @Override
        public boolean endBatchEdit() {
            if (mBatchEditNestCount > 1) {
                return super.endBatchEdit();
            }
            if (DEBUG) Log.i(TAG, "endBatchEdit");

            String diff = mCurrentState.getBackwardDeletedTextFrom(mPreBatchEditState);
            if (diff != null) {
                // Update selection first such that keyboard app gets what it expects.
                boolean retVal = super.endBatchEdit();

                if (mPreBatchEditState.hasAutocompleteText()) {
                    // Undo delete to retain the last character and only remove autocomplete text.
                    restoreBackspacedText(diff);
                }
                mLastEditWasTyping = false;
                mCurrentState.clearAutocompleteText();
                mSetAutocompleteState.copyFrom(mCurrentState);
                notifyAutocompleteTextStateChanged();
                return retVal;
            }
            if (!setAutocompleteSpan()) {
                mCurrentState.clearAutocompleteText();
            }
            boolean retVal = super.endBatchEdit();
            // Simply typed some characters or whole text selection has been overridden.
            if (mCurrentState.isForwardTypedFrom(mPreBatchEditState)
                    || (mPreBatchEditState.isWholeUserTextSelected()
                               && mCurrentState.getUserText().length() > 0
                               && mCurrentState.isCursorAtEndOfUserText())) {
                mLastEditWasTyping = true;
            }
            notifyAutocompleteTextStateChanged();
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
            return commitAutocompleteAndRunImeOperation(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return AutocompleteInputConnection.super.performEditorAction(editorAction);
                }
            });
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
