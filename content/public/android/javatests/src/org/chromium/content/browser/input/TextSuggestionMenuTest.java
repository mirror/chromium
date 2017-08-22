// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.input;

import android.support.test.filters.LargeTest;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.SuggestionSpan;
import android.view.View;
import android.widget.ListView;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.content.R;
import org.chromium.content.browser.ContentViewCore;
import org.chromium.content.browser.test.ContentJUnit4ClassRunner;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content.browser.test.util.TouchCommon;
import org.chromium.content_public.browser.WebContents;

import java.util.concurrent.TimeoutException;

/**
 * Integration tests for the spell check menu.
 */
@RunWith(ContentJUnit4ClassRunner.class)
public class TextSuggestionMenuTest {
    private static final String URL = "data:text/html, <div contenteditable id=\"div\" />";

    @Rule
    public ImeActivityTestRule mRule = new ImeActivityTestRule();

    @Before
    public void setUp() throws Throwable {
        mRule.setUp();
        mRule.fullyLoadUrl(URL);
    }

    @Test
    @LargeTest
    public void testDeleteMarkedWord() throws InterruptedException, Throwable, TimeoutException {
        final ContentViewCore cvc = mRule.getContentViewCore();
        WebContents webContents = cvc.getWebContents();

        DOMUtils.focusNode(webContents, "div");

        SpannableString textToCommit = new SpannableString("hello");
        String[] suggestions = {"goodbye"};
        SuggestionSpan suggestionSpan = new SuggestionSpan(mRule.getContentViewCore().getContext(),
                suggestions, SuggestionSpan.FLAG_EASY_CORRECT);
        textToCommit.setSpan(suggestionSpan, 0, 5, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        mRule.commitText(textToCommit, 0);

        DOMUtils.clickNode(cvc, "div");
        waitForMenuToShow(cvc);

        TouchCommon.singleClickView(getDeleteButton(cvc));
        Assert.assertEquals("", DOMUtils.getNodeContents(cvc.getWebContents(), "div"));
    }

    @Test
    @LargeTest
    public void testApplySuggestion() throws InterruptedException, Throwable, TimeoutException {
        final ContentViewCore cvc = mRule.getContentViewCore();
        WebContents webContents = cvc.getWebContents();

        DOMUtils.focusNode(webContents, "div");

        SpannableString textToCommit = new SpannableString("hello");
        String[] suggestions = {"goodbye"};
        SuggestionSpan suggestionSpan = new SuggestionSpan(mRule.getContentViewCore().getContext(),
                suggestions, SuggestionSpan.FLAG_EASY_CORRECT);
        textToCommit.setSpan(suggestionSpan, 0, 5, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        mRule.commitText(textToCommit, 0);

        DOMUtils.clickNode(cvc, "div");
        waitForMenuToShow(cvc);

        TouchCommon.singleClickView(getSuggestionButton(cvc, 0));

        // Applying a suggestion is apparently slower than deleting; checking the condition
        // immediately without polling fails.
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                try {
                    return DOMUtils.getNodeContents(cvc.getWebContents(), "div").equals("goodbye");
                } catch (InterruptedException | TimeoutException e) {
                    return false;
                }
            }
        });
    }

    private void waitForMenuToShow(ContentViewCore cvc) {
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                SuggestionsPopupWindow suggestionsPopupWindow =
                        cvc.getTextSuggestionHostForTesting().getSuggestionsPopupWindowForTesting();
                if (suggestionsPopupWindow == null) {
                    return false;
                }

                // On some test runs, even when suggestionsPopupWindow is non-null and
                // suggestionsPopupWindow.isShowing() returns true, the delete button hasn't been
                // measured yet and getWidth()/getHeight() return 0. This causes the menu button
                // click to instead fall on the "Add to dictionary" button. So we have to check that
                // this isn't happening.
                return getDeleteButton(cvc).getWidth() != 0;
            }
        });
    }

    private View getSuggestionButton(ContentViewCore cvc, int suggestionIndex) {
        View contentView = cvc.getTextSuggestionHostForTesting()
                                   .getSuggestionsPopupWindowForTesting()
                                   .getContentViewForTesting();
        ListView listView = (ListView) contentView.findViewById(R.id.suggestionContainer);
        return listView.getChildAt(suggestionIndex);
    }

    private View getDeleteButton(ContentViewCore cvc) {
        View contentView = cvc.getTextSuggestionHostForTesting()
                                   .getSuggestionsPopupWindowForTesting()
                                   .getContentViewForTesting();
        return contentView.findViewById(R.id.deleteButton);
    }
}
