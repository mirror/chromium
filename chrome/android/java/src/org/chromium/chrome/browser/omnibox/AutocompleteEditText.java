// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import android.content.Context;
import android.graphics.Rect;
import android.os.StrictMode;
import android.text.Editable;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.EditText;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.widget.VerticallyFixedEditText;

import java.util.concurrent.Callable;

/**
 * An {@link EditText} that shows autocomplete text at the end.
 */
public class AutocompleteEditText
        extends VerticallyFixedEditText implements AutocompleteEditTextModelBase.Delegate {
    private static final String TAG = "cr_AutocompleteEdit";

    private static final boolean DEBUG = false;
    private static final boolean USE_SPANNABLE_MODEL = true;

    private final AccessibilityManager mAccessibilityManager;

    private AutocompleteEditTextModelBase mModel;

    /**
     * Whether default TextView scrolling should be disabled because autocomplete has been added.
     * This allows the user entered text to be shown instead of the end of the autocomplete.
     */
    private boolean mDisableTextScrollingFromAutocomplete;

    private boolean mIgnoreImeForTest;

    public AutocompleteEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
        mModel = createModel();
        // We get these callbacks in View's constructor by which time we haven't initialized mModel.
        // So calling here to make up for it.
        mModel.onSelectionChanged(getSelectionStart(), getSelectionEnd());
        mModel.onTextChanged(getText(), 0, 0, 0);
        mModel.onSetText(getText());

        mAccessibilityManager =
                (AccessibilityManager) context.getSystemService(Context.ACCESSIBILITY_SERVICE);
    }

    @VisibleForTesting
    protected AutocompleteEditTextModelBase createModel() {
        if (USE_SPANNABLE_MODEL) {
            return new SpannableAutocompleteEditTextModel(this);
        } else {
            return new AutocompleteEditTextModel(this);
        }
    }

    /**
     * Sets whether text changes should trigger autocomplete.
     *
     * @param ignoreAutocomplete Whether text changes should be ignored and no auto complete
     *                           triggered.
     */
    public void setIgnoreTextChangesForAutocomplete(boolean ignoreAutocomplete) {
        if (mModel != null) mModel.setIgnoreTextChangeFromAutocomplete(ignoreAutocomplete);
    }

    /**
     * @return The user text without the autocomplete text.
     */
    public String getTextWithoutAutocomplete() {
        return mModel != null ? mModel.getTextWithoutAutocomplete() : getText().toString();
    }

    /** @return Text that includes autocomplete. */
    public String getTextWithAutocomplete() {
        return mModel != null ? mModel.getTextWithAutocomplete() : getText().toString();
    }

    /** @return Whether any autocomplete information is specified on the current text. */
    @VisibleForTesting
    public boolean hasAutocomplete() {
        return mModel != null ? mModel.hasAutocomplete() : false;
    }

    /**
     * Whether we want to be showing inline autocomplete results. We don't want to show them as the
     * user deletes input. Also if there is a composition (e.g. while using the Japanese IME),
     * we must not autocomplete or we'll destroy the composition.
     * @return Whether we want to be showing inline autocomplete results.
     */
    public boolean shouldAutocomplete() {
        return mModel != null ? mModel.shouldAutocomplete() : false;
    }

    @Override
    protected void onSelectionChanged(int selStart, int selEnd) {
        if (mModel != null) mModel.onSelectionChanged(selStart, selEnd);
        super.onSelectionChanged(selStart, selEnd);
    }

    @Override
    protected void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
        if (mModel != null) mModel.onFocusChanged(focused);
        super.onFocusChanged(focused, direction, previouslyFocusedRect);
    }

    @Override
    public boolean bringPointIntoView(int offset) {
        if (mDisableTextScrollingFromAutocomplete) return false;
        return super.bringPointIntoView(offset);
    }

    @Override
    public boolean onPreDraw() {
        boolean retVal = super.onPreDraw();
        if (mDisableTextScrollingFromAutocomplete) {
            // super.onPreDraw will put the selection at the end of the text selection, but
            // in the case of autocomplete we want the last typed character to be shown, which
            // is the start of selection.
            mDisableTextScrollingFromAutocomplete = false;
            bringPointIntoView(getSelectionStart());
            retVal = true;
        }
        return retVal;
    }

    /** Call this when text is pasted. */
    public void onPaste() {
        if (mModel != null) mModel.onPaste();
    }

    /**
     * Autocompletes the text and selects the text that was not entered by the user. Using append()
     * instead of setText() to preserve the soft-keyboard layout.
     * @param userText user The text entered by the user.
     * @param inlineAutocompleteText The suggested autocompletion for the user's text.
     */
    public void setAutocompleteText(CharSequence userText, CharSequence inlineAutocompleteText) {
        boolean emptyAutocomplete = TextUtils.isEmpty(inlineAutocompleteText);
        if (!emptyAutocomplete) mDisableTextScrollingFromAutocomplete = true;
        if (mModel != null) mModel.setAutocompleteText(userText, inlineAutocompleteText);
    }

    /**
     * Returns the length of the autocomplete text currently displayed, zero if none is
     * currently displayed.
     */
    public int getAutocompleteLength() {
        return mModel != null ? mModel.getAutocompleteText().length() : 0;
    }

    @Override
    protected void onTextChanged(CharSequence text, int start, int lengthBefore, int lengthAfter) {
        super.onTextChanged(text, start, lengthBefore, lengthAfter);
        if (mModel != null) mModel.onTextChanged(text, start, lengthBefore, lengthAfter);
    }

    @Override
    public void setText(CharSequence text, BufferType type) {
        if (DEBUG) Log.i(TAG, "setText -- text: %s", text);
        mDisableTextScrollingFromAutocomplete = false;

        // Avoid setting the same text as it will mess up the scroll/cursor position.
        // Setting the text is also quite expensive, so only do it when the text has changed
        // (since we apply spans when the view is not focused, we only optimize this when the
        // text is being edited).
        if (!TextUtils.equals(getEditableText(), text)) {
            // Certain OEM implementations of setText trigger disk reads. crbug.com/633298
            StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
            try {
                super.setText(text, type);
            } finally {
                StrictMode.setThreadPolicy(oldPolicy);
            }
        }
        if (mModel != null) mModel.onSetText(text);
    }

    @Override
    public void sendAccessibilityEventUnchecked(AccessibilityEvent event) {
        if (mModel != null && mModel.shouldIgnoreTextChangeFromAutocomplete()) {
            if (event.getEventType() == AccessibilityEvent.TYPE_VIEW_TEXT_SELECTION_CHANGED
                    || event.getEventType() == AccessibilityEvent.TYPE_VIEW_TEXT_CHANGED) {
                if (DEBUG) Log.i(TAG, "Ignoring accessibility event from autocomplete.");
                return;
            }
        }
        super.sendAccessibilityEventUnchecked(event);
    }

    @Override
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        // Certain OEM implementations of onInitializeAccessibilityNodeInfo trigger disk reads
        // to access the clipboard.  crbug.com/640993
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        try {
            super.onInitializeAccessibilityNodeInfo(info);
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }
    }

    @VisibleForTesting
    public InputConnection getInputConnection() {
        return mModel != null ? mModel.getInputConnection() : null;
    }

    @VisibleForTesting
    public void setIgnoreImeForTest(boolean ignore) {
        mIgnoreImeForTest = ignore;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return createInputConnection(super.onCreateInputConnection(outAttrs));
    }

    @VisibleForTesting
    public InputConnection createInputConnection(InputConnection target) {
        if (mModel != null) mModel.onCreateInputConnection(target);
        if (mIgnoreImeForTest) return null;
        return mModel != null ? mModel.getInputConnection() : null;
    }

    @Override
    public boolean dispatchKeyEvent(final KeyEvent event) {
        if (mIgnoreImeForTest) return true;
        return mModel.dispatchKeyEvent(event, new Callable<Boolean>() {
            @Override
            public Boolean call() throws Exception {
                return AutocompleteEditText.super.dispatchKeyEvent(event);
            }
        });
    }

    /**
     * @return Whether the current UrlBar input has been pasted from the clipboard.
     */
    public boolean isPastedText() {
        return mModel != null ? mModel.isPastedText() : false;
    }

    @Override
    public void replaceAllTextFromAutocomplete(String text) {
        assert false; // make sure that this method is properly overridden.
    }

    @Override
    public void onAutocompleteTextStateChanged(boolean textDeleted, boolean updateDisplay) {
        assert false; // make sure that this method is properly overridden.
    }

    @Override
    public void onNoChangeTypingAccessibilityEvent(int selectionStart) {
        if (DEBUG) Log.i(TAG, "onNoChangeTypingAccessibilityEvent: " + selectionStart);
        // Since the text isn't changing, TalkBack won't read out the typed characters.
        // To work around this, explicitly send an accessibility event. crbug.com/416595
        Editable currentText = getText();
        if (mAccessibilityManager != null && mAccessibilityManager.isEnabled()) {
            AccessibilityEvent event =
                    AccessibilityEvent.obtain(AccessibilityEvent.TYPE_VIEW_TEXT_CHANGED);
            event.setFromIndex(selectionStart);
            event.setRemovedCount(0);
            event.setAddedCount(1);
            event.setBeforeText(currentText.toString().substring(0, selectionStart));
            sendAccessibilityEventUnchecked(event);
        }
    }

    @Override
    public void onUpdateSelectionForTesting(int selStart, int selEnd) {}
}
