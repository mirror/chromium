// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.input;

import android.support.test.filters.SmallTest;
import android.view.inputmethod.EditorInfo;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.blink_public.web.WebTextInputMode;
import org.chromium.content.browser.test.ContentJUnit4ClassRunner;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.ui.base.ime.TextInputType;

/**
 * IME (input method editor) and text input tests with experimental features enabled.
 */
@RunWith(ContentJUnit4ClassRunner.class)
@CommandLineFlags.Add({"expose-internals-for-testing", "enable-experimental-web-platform-features"})
public class ImeExperimentalFeaturesTest {
    @Rule
    public ImeActivityTestRule mRule = new ImeActivityTestRule();

    @Before
    public void setUp() throws Exception {
        mRule.setUpForUrl(ImeActivityTestRule.INPUT_FORM_HTML);
    }

    @Test
    @SmallTest
    @Feature({"TextInput"})
    public void testShowAndHideInputMode() throws Exception {
        mRule.focusElement("contenteditable_none", false);

        // hideSoftKeyboard(), mRule.restartInput()
        mRule.waitForKeyboardStates(0, 1, 1, new Integer[] {}, new Integer[] {});

        // When input connection is null, we still need to set flags to prevent InputMethodService
        // from entering fullscreen mode and from opening custom UI.
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return mRule.getInputConnection() == null;
            }
        });
        Assert.assertTrue(
                (mRule.getConnectionFactory().getOutAttrs().imeOptions
                        & (EditorInfo.IME_FLAG_NO_FULLSCREEN | EditorInfo.IME_FLAG_NO_EXTRACT_UI))
                != 0);

        // showSoftInput(), mRule.restartInput()
        mRule.focusElement("contenteditable_text");
        mRule.waitForKeyboardStates(1, 1, 2, new Integer[] {TextInputType.CONTENT_EDITABLE},
                new Integer[] {WebTextInputMode.TEXT});
        Assert.assertNotNull(mRule.getInputMethodManagerWrapper().getInputConnection());

        mRule.focusElement("contenteditable_tel");
        // Hide should never be called here. Otherwise we will see a flicker. Restarted to
        // reset internal states to handle the new input form.
        mRule.waitForKeyboardStates(2, 1, 3,
                new Integer[] {TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE},
                new Integer[] {WebTextInputMode.TEXT, WebTextInputMode.TEL});

        mRule.focusElement("contenteditable_url");
        mRule.waitForKeyboardStates(3, 1, 4,
                new Integer[] {TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE,
                        TextInputType.CONTENT_EDITABLE},
                new Integer[] {WebTextInputMode.TEXT, WebTextInputMode.TEL, WebTextInputMode.URL});

        mRule.focusElement("contenteditable_email");
        mRule.waitForKeyboardStates(4, 1, 5,
                new Integer[] {TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE,
                        TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE},
                new Integer[] {WebTextInputMode.TEXT, WebTextInputMode.TEL, WebTextInputMode.URL,
                        WebTextInputMode.EMAIL});

        mRule.focusElement("contenteditable_numeric");
        mRule.waitForKeyboardStates(5, 1, 6,
                new Integer[] {TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE,
                        TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE,
                        TextInputType.CONTENT_EDITABLE},
                new Integer[] {WebTextInputMode.TEXT, WebTextInputMode.TEL, WebTextInputMode.URL,
                        WebTextInputMode.EMAIL, WebTextInputMode.NUMERIC});

        mRule.focusElement("contenteditable_decimal");
        mRule.waitForKeyboardStates(6, 1, 7,
                new Integer[] {TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE,
                        TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE,
                        TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE},
                new Integer[] {WebTextInputMode.TEXT, WebTextInputMode.TEL, WebTextInputMode.URL,
                        WebTextInputMode.EMAIL, WebTextInputMode.NUMERIC,
                        WebTextInputMode.DECIMAL});

        mRule.focusElement("contenteditable_search");
        mRule.waitForKeyboardStates(7, 1, 8,
                new Integer[] {TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE,
                        TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE,
                        TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE,
                        TextInputType.CONTENT_EDITABLE},
                new Integer[] {WebTextInputMode.TEXT, WebTextInputMode.TEL, WebTextInputMode.URL,
                        WebTextInputMode.EMAIL, WebTextInputMode.NUMERIC, WebTextInputMode.DECIMAL,
                        WebTextInputMode.SEARCH});

        mRule.focusElement("contenteditable_none", false);
        mRule.waitForKeyboardStates(7, 2, 9,
                new Integer[] {TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE,
                        TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE,
                        TextInputType.CONTENT_EDITABLE, TextInputType.CONTENT_EDITABLE,
                        TextInputType.CONTENT_EDITABLE},
                new Integer[] {WebTextInputMode.TEXT, WebTextInputMode.TEL, WebTextInputMode.URL,
                        WebTextInputMode.EMAIL, WebTextInputMode.NUMERIC, WebTextInputMode.DECIMAL,
                        WebTextInputMode.SEARCH});
    }
}
