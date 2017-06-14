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
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

import org.junit.Before;
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
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class SpannableAutocompleteEditTextTest {
    private static final String TAG = "cr_AutocompleteEdit";

    private static final boolean DEBUG = false;

    private InOrder mInOrder;
    private AutocompleteEditText mAutocomplete;
    private SpannableAutocompleteEditTextModel mModel;
    private Context mContext;
    private InputConnection mInputConnection;
    private BaseInputConnection mDummyTargetInputConnection;
    private Verifier mVerifier;

    // Limits the target of inOrder#verify.
    private static class Verifier {
        public void onAutocompleteTextStateChanged(boolean textDeleted, boolean updateDisplay) {
            if (DEBUG) Log.i(TAG, "onAutocompleteTextStateChanged");
        }
    }

    private class TestAutocompleteEditText extends AutocompleteEditText {
        public TestAutocompleteEditText(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        @Override
        protected AutocompleteEditTextModelBase createModel() {
            return new SpannableAutocompleteEditTextModel(this);
        }

        @Override
        public void onAutocompleteTextStateChanged(boolean textDeleted, boolean updateDisplay) {
            if (mVerifier != null) {
                mVerifier.onAutocompleteTextStateChanged(textDeleted, updateDisplay);
            }
        }

        @Override
        public boolean hasFocus() {
            return true;
        }
    }

    @Before
    public void setUp() throws Exception {
        if (DEBUG) {
            ShadowLog.stream = System.out;
            Log.i(TAG, "setUp started.");
        }
        MockitoAnnotations.initMocks(this);
        mContext = RuntimeEnvironment.application;
        mVerifier = spy(new Verifier());
        mAutocomplete = spy(new TestAutocompleteEditText(mContext, null));
        assertNotNull(mAutocomplete);
        // Note: this cannot catch the first {@link
        // AutocompleteEditText#onAutocompleteTextStateChanged(boolean, boolean)}, which is caused
        // by View constructor's call to setText().
        mInOrder = inOrder(mVerifier);
        mAutocomplete.onCreateInputConnection(new EditorInfo());
        mInputConnection = mAutocomplete.getInputConnection();
        assertNotNull(mInputConnection);
        mInOrder.verifyNoMoreInteractions();
        if (DEBUG) Log.i(TAG, "setUp finished.");
    }

    private void assertTexts(String userText, String autocompleteText) {
        assertEquals(userText, mAutocomplete.getTextWithoutAutocomplete());
        assertEquals(userText + autocompleteText, mAutocomplete.getTextWithAutocomplete());
        assertEquals(autocompleteText.length(), mAutocomplete.getAutocompleteLength());
        assertEquals(!TextUtils.isEmpty(autocompleteText), mAutocomplete.hasAutocomplete());
    }

    @Test
    public void testAppend_CommitText() {
        // Feeder should call this at the beginning.
        mAutocomplete.setIgnoreTextChangesForAutocomplete(false);
        // User types "h".
        assertTrue(mInputConnection.commitText("h", 1));
        mInOrder.verify(mVerifier).onAutocompleteTextStateChanged(false, true);
        // User types "hello".
        assertTrue(mInputConnection.commitText("ello", 1));
        mInOrder.verify(mVerifier).onAutocompleteTextStateChanged(false, true);
        mInOrder.verifyNoMoreInteractions();
        // The controller kicks in.
        mAutocomplete.setAutocompleteText("hello", " world");
        assertTexts("hello", " world");

        assertTrue(mInputConnection.beginBatchEdit());
        assertTrue(mInputConnection.commitText(" ", 1));
        // Autocomplete text is not redrawn until the batch edit ends.
        assertEquals("hello ", mAutocomplete.getTextWithAutocomplete());

        mInOrder.verifyNoMoreInteractions();
        assertTrue(mInputConnection.endBatchEdit());
        // Autocomplete text gets redrawn.
        assertTexts("hello ", "world");

        mInOrder.verify(mVerifier).onAutocompleteTextStateChanged(false, true);
        mAutocomplete.setAutocompleteText("hello ", "world");
        assertTexts("hello ", "world");
        mInOrder.verifyNoMoreInteractions();
    }
}