// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.support.annotation.DrawableRes;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.filters.SmallTest;
import android.support.v7.widget.RecyclerView;
import android.view.View;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ntp.cards.NewTabPageAdapter;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;
import org.chromium.chrome.test.BottomSheetTestRule;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.RenderTestRule;
import org.chromium.chrome.test.util.browser.suggestions.FakeSuggestionsSource;
import org.chromium.chrome.test.util.browser.suggestions.SuggestionsDependenciesRule;
import org.chromium.net.test.EmbeddedTestServer;

import java.io.IOException;
import java.util.List;

/**
 * Tests for the appearance of Article Snippets.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags
        .Add({ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG})
@CommandLineFlags.Remove({BottomSheetTestRule.ENABLE_CHROME_HOME})
public class ContextualSuggestionsTest {
    private static final int NUMBER_OF_SUGGESTIONS = 10;
    private static final String TEST_PAGE = "/chrome/test/data/android/test.html";
    private static final int CAROUSEL_POSITION_IN_RECYCLER_VIEW = 0;

    @Rule
    public SuggestionsDependenciesRule mSuggestionsDeps = new SuggestionsDependenciesRule();
    @Rule
    public SuggestionsBottomSheetTestRule mActivityRule = new SuggestionsBottomSheetTestRule();
    @Rule
    public RenderTestRule mRenderTestRule =
            new RenderTestRule("chrome/test/data/android/render_tests");

    private FakeSuggestionsSource mSnippetsSource;
    private SuggestionsRecyclerView mRecyclerView;

    @Before
    public void setUp() throws Exception {
        mSnippetsSource = new FakeSuggestionsSource();
        mSnippetsSource.setThumbnailForContextualSuggestions(getBitmap(R.drawable.star_green));
        mSuggestionsDeps.getFactory().suggestionsSource = mSnippetsSource;

        mActivityRule.startMainActivityOnBottomSheet(BottomSheet.SHEET_STATE_PEEK);

        mRecyclerView = mActivityRule.getRecyclerView();
    }

    @Test
    @SmallTest
    @Feature({"ContextualSuggestions"})
    @CommandLineFlags.Add({"enable-features=" + ChromeFeatureList.CHROME_HOME})
    public void testCarouselIsNotShownWhenFlagIsDisabled() throws IOException {
        Assert.assertTrue(ChromeFeatureList.isEnabled(ChromeFeatureList.CHROME_HOME));
        Assert.assertFalse(
                ChromeFeatureList.isEnabled(ChromeFeatureList.CONTEXTUAL_SUGGESTIONS_CAROUSEL));

        mActivityRule.setSheetState(BottomSheet.SHEET_STATE_HALF, false);
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        NewTabPageAdapter adapter = mActivityRule.getAdapter();

        SuggestionsCarousel carousel = adapter.getSuggesitonsCarousel();
        Assert.assertNull(carousel);
    }

    @Test
    @SmallTest
    @Feature({"ContextualSuggestions"})
    @CommandLineFlags.Add({"enable-features=" + ChromeFeatureList.CHROME_HOME + ","
            + ChromeFeatureList.CONTEXTUAL_SUGGESTIONS_CAROUSEL})
    public void testCarouselIsShownWhenReceivedSuggestionsFromNative() throws Exception {
        mActivityRule.setSheetState(BottomSheet.SHEET_STATE_HALF, false);
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        NewTabPageAdapter adapter = mActivityRule.getAdapter();

        final SuggestionsCarousel carousel = adapter.getSuggesitonsCarousel();
        Assert.assertNotNull(carousel);

        View carouselRecyclerView = mRecyclerView.getChildAt(CAROUSEL_POSITION_IN_RECYCLER_VIEW);

        Assert.assertTrue(carouselRecyclerView instanceof RecyclerView);
        Assert.assertEquals(carouselRecyclerView.getVisibility(), View.VISIBLE);
    }

    @Test
    @SmallTest
    @Feature({"ContextualSuggestions"})
    @CommandLineFlags.Add({"enable-features=" + ChromeFeatureList.CHROME_HOME + ","
            + ChromeFeatureList.CONTEXTUAL_SUGGESTIONS_CAROUSEL})
    public void testCarouselDisplaysTheRightNumberOfSuggestions() throws Exception {
        Assert.assertTrue(ChromeFeatureList.isEnabled(ChromeFeatureList.CHROME_HOME));
        Assert.assertTrue(
                ChromeFeatureList.isEnabled(ChromeFeatureList.CONTEXTUAL_SUGGESTIONS_CAROUSEL));

        mSnippetsSource.createAndSetContextualSuggestions(NUMBER_OF_SUGGESTIONS);

        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(
                InstrumentationRegistry.getInstrumentation().getContext());

        String testUrl = testServer.getURL(TEST_PAGE);
        mActivityRule.loadUrl(testUrl);
        mActivityRule.setSheetState(BottomSheet.SHEET_STATE_HALF, false);

        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        mActivityRule.getObserver().mOpenedCallbackHelper.waitForCallback(0);

        final RecyclerView carouselRecyclerView =
                (RecyclerView) mRecyclerView.getChildAt(CAROUSEL_POSITION_IN_RECYCLER_VIEW);
        List<SnippetArticle> suggestions =
                ((SuggestionsCarouselAdapter) carouselRecyclerView.getAdapter()).getSuggestions();

        Assert.assertNotNull(suggestions);
        Assert.assertEquals(NUMBER_OF_SUGGESTIONS, suggestions.size());
    }

    @Test
    @MediumTest
    @Feature({"ContextualSuggestions", "RenderTest"})
    @CommandLineFlags.Add({"enable-features=" + ChromeFeatureList.CHROME_HOME + ","
            + ChromeFeatureList.CONTEXTUAL_SUGGESTIONS_CAROUSEL})
    public void testCardAppearance() throws Exception {
        mSnippetsSource.createAndSetContextualSuggestions(NUMBER_OF_SUGGESTIONS);

        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(
                InstrumentationRegistry.getInstrumentation().getContext());

        String testUrl = testServer.getURL(TEST_PAGE);
        mActivityRule.loadUrl(testUrl);
        mActivityRule.setSheetState(BottomSheet.SHEET_STATE_HALF, false);

        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        mActivityRule.getObserver().mOpenedCallbackHelper.waitForCallback(0);

        final RecyclerView carouselRecyclerView =
                (RecyclerView) mRecyclerView.getChildAt(CAROUSEL_POSITION_IN_RECYCLER_VIEW);

        mRenderTestRule.render(carouselRecyclerView.getChildAt(0), "contextual_suggestions_card");
    }

    private Bitmap getBitmap(@DrawableRes int resId) {
        return BitmapFactory.decodeResource(
                mActivityRule.getInstrumentation().getTargetContext().getResources(), resId);
    }
}
