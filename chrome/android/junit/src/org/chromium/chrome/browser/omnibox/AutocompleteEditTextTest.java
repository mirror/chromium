// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.omnibox;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.spy;

import android.content.Context;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InOrder;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLog;

import org.chromium.base.Log;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * A robolectric test for {@link AutocompleteEditText} class.
 * TODO(changwan): switch to ParameterizedRobolectricTest once crbug.com/733324 is fixed.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class AutocompleteEditTextTest {
    private static final String TAG = "cr_AutocompleteTest";

    private static final boolean DEBUG = false;

    private boolean mUseSpannableModel;
    private InOrder mInOrder;
    private AutocompleteEditText mAutocomplete;
    private Context mContext;
    private InputConnection mInputConnection;
    private Verifier mVerifier;

    // Limits the target of InOrder#verify.
    private static class Verifier {
        public void onAutocompleteTextStateChanged(boolean updateDisplay) {
            if (DEBUG) Log.i(TAG, "onAutocompleteTextStateChanged");
        }

        public void onUpdateSelection(int selStart, int selEnd) {
            if (DEBUG) Log.i(TAG, "onUpdateSelection: [%d,%d]", selStart, selEnd);
        }
    }

    private class TestAutocompleteEditText extends AutocompleteEditText {
        private int mLastSelStart;
        private int mLastSelEnd;

        public TestAutocompleteEditText(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        @Override
        protected AutocompleteEditTextModelBase createModel() {
            return mUseSpannableModel ? new SpannableAutocompleteEditTextModel(this)
                                      : new AutocompleteEditTextModel(this);
        }

        @Override
        public void onAutocompleteTextStateChanged(boolean updateDisplay) {
            // This function is called in super(), so mVerifier may be null.
            if (mVerifier != null) {
                mVerifier.onAutocompleteTextStateChanged(updateDisplay);
            }
        }

        @Override
        public void onUpdateSelectionForTesting(int selStart, int selEnd) {
            // This function is called in super(), so mVerifier may be null.
            if (mVerifier != null) {
                mVerifier.onUpdateSelection(selStart, selEnd);
            }
        }

        @Override
        public boolean hasFocus() {
            return true;
        }
    }

    public AutocompleteEditTextTest() {
        if (DEBUG) ShadowLog.stream = System.out;
    }

    private void setUp() {
        if (DEBUG) Log.i(TAG, "setUp started.");
        MockitoAnnotations.initMocks(this);
        mContext = RuntimeEnvironment.application;
        mVerifier = spy(new Verifier());
        mAutocomplete = spy(new TestAutocompleteEditText(mContext, null));
        assertNotNull(mAutocomplete);
        mInOrder = inOrder(mVerifier);
        mAutocomplete.onCreateInputConnection(new EditorInfo());
        mInputConnection = mAutocomplete.getInputConnection();
        assertNotNull(mInputConnection);
        mInOrder.verifyNoMoreInteractions();
        // Feeder should call this at the beginning.
        mAutocomplete.setIgnoreTextChangesForAutocomplete(false);

        if (DEBUG) Log.i(TAG, "setUp finished.");
    }

    private void assertTexts(String userText, String autocompleteText) {
        assertEquals(userText, mAutocomplete.getTextWithoutAutocomplete());
        assertEquals(userText + autocompleteText, mAutocomplete.getTextWithAutocomplete());
        assertEquals(autocompleteText.length(), mAutocomplete.getAutocompleteLength());
        assertEquals(!TextUtils.isEmpty(autocompleteText), mAutocomplete.hasAutocomplete());
    }

    @Test
    public void testAppend_CommitTextWithSpannableModel() {
        internalTestAppend_CommitText(true);
    }

    @Test
    public void testAppend_CommitTextWithoutSpannableModel() {
        internalTestAppend_CommitText(false);
    }

    @Test
    public void testAppend_SetComposingTextWithSpannableModel() {
        internalTestAppend_SetComposingText(true);
    }

    @Test
    public void testAppend_SetComposingTextWithoutSpannableModel() {
        internalTestAppend_SetComposingText(false);
    }

    @Test
    public void testDelete_SetComposingTextWithSpannableModel() {
        mUseSpannableModel = true;
        setUp();

        // User types "hello".
        assertTrue(mInputConnection.setComposingText("hello", 1));

        mInOrder.verify(mVerifier).onAutocompleteTextStateChanged(true);
        mInOrder.verify(mVerifier).onUpdateSelection(5, 5);
        mInOrder.verifyNoMoreInteractions();
        assertEquals(true, mAutocomplete.shouldAutocomplete());
        // The controller kicks in.
        mAutocomplete.setAutocompleteText("hello", " world");
        assertTexts("hello", " world");
        mInOrder.verifyNoMoreInteractions();

        // User deletes 'o'.
        assertTrue(mInputConnection.setComposingText("hell", 1));
        mInOrder.verify(mVerifier).onUpdateSelection(4, 4);
        mInOrder.verify(mVerifier).onUpdateSelection(5, 5);
        mInOrder.verify(mVerifier).onAutocompleteTextStateChanged(false);
        mInOrder.verifyNoMoreInteractions();
        assertTexts("hello", "");

        // Keyboard app checks the current state.
        assertEquals("hello", mInputConnection.getTextBeforeCursor(10, 0));
        mInOrder.verifyNoMoreInteractions();
    }

    private void internalTestAppend_CommitText(boolean useSpannableModel) {
        mUseSpannableModel = useSpannableModel;
        setUp();

        // User types "h".
        assertTrue(mInputConnection.commitText("h", 1));
        mInOrder.verify(mVerifier).onAutocompleteTextStateChanged(true);
        mInOrder.verify(mVerifier).onUpdateSelection(1, 1);
        mInOrder.verifyNoMoreInteractions();
        assertTrue(mAutocomplete.shouldAutocomplete());

        // The controller kicks in.
        mAutocomplete.setAutocompleteText("h", "ello world");
        // The non-spannable model changes selection in two steps.
        if (!mUseSpannableModel) {
            mInOrder.verify(mVerifier).onUpdateSelection(11, 11);
            mInOrder.verify(mVerifier).onUpdateSelection(1, 11);
        }
        mInOrder.verifyNoMoreInteractions();

        // User types "hello".
        assertTrue(mInputConnection.commitText("ello", 1));
        mInOrder.verify(mVerifier).onAutocompleteTextStateChanged(true);
        mInOrder.verify(mVerifier).onUpdateSelection(5, 5);
        mInOrder.verifyNoMoreInteractions();
        // The controller kicks in.
        mAutocomplete.setAutocompleteText("hello", " world");
        if (!mUseSpannableModel) {
            mInOrder.verify(mVerifier).onUpdateSelection(11, 11);
            mInOrder.verify(mVerifier).onUpdateSelection(5, 11);
        }
        mInOrder.verifyNoMoreInteractions();
        assertTexts("hello", " world");

        // User types a space.
        assertTrue(mInputConnection.beginBatchEdit());
        assertTrue(mInputConnection.commitText(" ", 1));

        if (mUseSpannableModel) {
            // Under spannable model, autocomplete text is removed from the screen but internally
            // kept until the batch edit ends.
            assertEquals("hello ", mAutocomplete.getTextWithAutocomplete());
            assertEquals(6, mAutocomplete.getAutocompleteLength());
        } else {
            assertTexts("hello ", "world");
        }

        mInOrder.verifyNoMoreInteractions();
        assertTrue(mInputConnection.endBatchEdit());

        // Autocomplete text gets redrawn.
        assertTexts("hello ", "world");
        if (mUseSpannableModel) {
            mInOrder.verify(mVerifier).onUpdateSelection(6, 6);
        } else {
            mInOrder.verify(mVerifier).onUpdateSelection(6, 11);
        }

        mAutocomplete.setAutocompleteText("hello ", "world");
        assertTexts("hello ", "world");
        mInOrder.verifyNoMoreInteractions();
    }

    private void internalTestAppend_SetComposingText(boolean useSpannableModel) {
        mUseSpannableModel = useSpannableModel;
        setUp();

        // User types "h".
        assertTrue(mInputConnection.setComposingText("h", 1));

        mInOrder.verify(mVerifier).onAutocompleteTextStateChanged(true);
        mInOrder.verify(mVerifier).onUpdateSelection(1, 1);
        mInOrder.verifyNoMoreInteractions();

        // The old model does not allow autocompletion here.
        assertEquals(mUseSpannableModel, mAutocomplete.shouldAutocomplete());
        if (mUseSpannableModel) {
            // The controller kicks in.
            mAutocomplete.setAutocompleteText("h", "ello world");
            assertTexts("h", "ello world");
        } else {
            assertTexts("h", "");
        }
        mInOrder.verifyNoMoreInteractions();

        // User types "hello".
        assertTrue(mInputConnection.setComposingText("hello", 1));
        mInOrder.verify(mVerifier).onAutocompleteTextStateChanged(true);
        mInOrder.verify(mVerifier).onUpdateSelection(5, 5);
        mInOrder.verifyNoMoreInteractions();
        if (mUseSpannableModel) {
            assertTexts("hello", " world");
        } else {
            // The old model does not work with composition.
            assertTexts("hello", "");
        }

        // The old model does not allow autocompletion here.
        assertEquals(mUseSpannableModel, mAutocomplete.shouldAutocomplete());
        if (mUseSpannableModel) {
            // The controller kicks in.
            mAutocomplete.setAutocompleteText("hello", " world");
            assertTexts("hello", " world");
        } else {
            assertTexts("hello", "");
        }
        mInOrder.verifyNoMoreInteractions();

        // User types a space.
        assertTrue(mInputConnection.beginBatchEdit());
        assertTrue(mInputConnection.finishComposingText());
        assertTrue(mInputConnection.commitText(" ", 1));

        if (mUseSpannableModel) {
            // Under spannable model, autocomplete text is removed from the screen but internally
            // kept until the batch edit ends.
            assertEquals("hello ", mAutocomplete.getTextWithAutocomplete());
            assertEquals(6, mAutocomplete.getAutocompleteLength());
        } else {
            assertTexts("hello ", "");
        }

        mInOrder.verifyNoMoreInteractions();
        assertTrue(mInputConnection.endBatchEdit());
        mInOrder.verify(mVerifier).onAutocompleteTextStateChanged(true);
        mInOrder.verify(mVerifier).onUpdateSelection(6, 6);
        mInOrder.verifyNoMoreInteractions();

        if (mUseSpannableModel) {
            // Autocomplete text has been drawn at endBatchEdit().
            assertTexts("hello ", "world");
        } else {
            assertTexts("hello ", "");
        }

        // The old model can also autocomplete now.
        assertTrue(mAutocomplete.shouldAutocomplete());
        mAutocomplete.setAutocompleteText("hello ", "world");
        assertTexts("hello ", "world");
        if (!mUseSpannableModel) {
            mInOrder.verify(mVerifier).onUpdateSelection(6, 11);
        }
        mInOrder.verifyNoMoreInteractions();
    }
}
