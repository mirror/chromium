// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.view.View;
import android.view.View.MeasureSpec;
import android.widget.Space;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.ntp.cards.NewTabPageRecyclerView;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.NewTabPageTestUtils;
import org.chromium.chrome.test.util.browser.RecyclerViewTestUtils;
import org.chromium.chrome.test.util.browser.suggestions.FakeSuggestionsSource;
import org.chromium.chrome.test.util.browser.suggestions.SuggestionsDependenciesRule;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.Arrays;

/**
 * Instrumentation tests for the {@link TileGridLayout} on the New Tab Page.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({
        ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG,
})
@RetryOnFailure
public class TileGridLayoutTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    @Rule
    public SuggestionsDependenciesRule mSuggestionsDeps = new SuggestionsDependenciesRule();

    private static final String HOME_PAGE_URL = "http://ho.me/";

    private static final String[] FAKE_MOST_VISITED_URLS =
            new String[] {"/chrome/test/data/android/navigate/one.html",
                    "/chrome/test/data/android/navigate/two.html",
                    "/chrome/test/data/android/navigate/three.html",
                    "/chrome/test/data/android/navigate/four.html"};

    private Context mContext;
    private NewTabPage mNtp;
    private String[] mSiteSuggestionUrls;
    private FakeMostVisitedSites mMostVisitedSites;
    private EmbeddedTestServer mTestServer;

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
        mContext.setTheme(R.style.MainTheme);

        mTestServer = EmbeddedTestServer.createAndStartServer(mContext);

        mSiteSuggestionUrls = mTestServer.getURLs(FAKE_MOST_VISITED_URLS);
        String[] tmpSiteSuggestionUrls = new String[mSiteSuggestionUrls.length + 1];
        System.arraycopy(
                mSiteSuggestionUrls, 0, tmpSiteSuggestionUrls, 0, mSiteSuggestionUrls.length);
        tmpSiteSuggestionUrls[tmpSiteSuggestionUrls.length - 1] = HOME_PAGE_URL;
        mSiteSuggestionUrls = tmpSiteSuggestionUrls;

        mMostVisitedSites = new FakeMostVisitedSites();
        mSuggestionsDeps.getFactory().mostVisitedSites = mMostVisitedSites;

        String[] whitelistIconPaths = new String[mSiteSuggestionUrls.length];
        Arrays.fill(whitelistIconPaths, "");

        int[] sources = new int[mSiteSuggestionUrls.length];
        Arrays.fill(sources, TileSource.TOP_SITES);
        sources[mSiteSuggestionUrls.length - 1] = TileSource.HOMEPAGE;

        mMostVisitedSites.setTileSuggestions(
                mSiteSuggestionUrls, mSiteSuggestionUrls.clone(), whitelistIconPaths, sources);

        mSuggestionsDeps.getFactory().suggestionsSource = new FakeSuggestionsSource();

        mActivityTestRule.startMainActivityWithURL(UrlConstants.NTP_URL);
        Tab mTab = mActivityTestRule.getActivity().getActivityTab();
        NewTabPageTestUtils.waitForNtpLoaded(mTab);

        Assert.assertTrue(mTab.getNativePage() instanceof NewTabPage);
        mNtp = (NewTabPage) mTab.getNativePage();

        RecyclerViewTestUtils.waitForStableRecyclerView(getRecyclerView());
    }

    @After
    public void tearDown() throws Exception {
        mTestServer.stopAndDestroyServer();
    }

    @Test
    @MediumTest
    @Feature({"NewTabPage"})
    public void testNumColumnsOnDifferentScreenSizes() {
        int[] screenDimensions = {120, 240, 320, 460, 720, 960, 1280, 1920};
        for (int viewWidth : screenDimensions) {
            for (int viewHeight : screenDimensions) {
                runNumColumnsTest(viewWidth, viewHeight);
            }
        }
    }

    private void runNumColumnsTest(int viewWidth, int viewHeight) {
        TileGridLayout layout = new TileGridLayout(mContext, null);
        layout.setMaxRows(1);

        View primary = new Space(mContext);
        primary.setMinimumWidth(viewWidth);
        primary.setMinimumHeight(viewHeight);
        layout.addView(primary);

        // Trigger the measurement & layout algorithms.
        int parentWidthSpec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.EXACTLY);
        int parentHeightSpec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
        layout.measure(parentWidthSpec, parentHeightSpec);
        layout.layout(0, 0, layout.getMeasuredWidth(), layout.getMeasuredHeight());

        // Confirm that the primary View is in the correct place.
        int visibleTileViews = 0;
        boolean hasHomePage = false;
        for (int i = 0; i < layout.getChildCount(); i++) {
            TileView tileView = (TileView) layout.getChildAt(i);
            if (tileView.getVisibility() == View.VISIBLE) {
                visibleTileViews++;
            }
            if (HOME_PAGE_URL.equals(tileView.getUrl())) {
                hasHomePage = true;
            }
        }
        Assert.assertTrue(hasHomePage);
        Assert.assertEquals(TileGridLayout.calculateNumColumns(), visibleTileViews);
    }

    private NewTabPageRecyclerView getRecyclerView() {
        return mNtp.getNewTabPageView().getRecyclerView();
    }
}
