// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.support.annotation.Nullable;
import android.support.test.filters.SmallTest;
import android.view.ViewGroup;

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
import org.chromium.net.test.EmbeddedTestServerRule;

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

    @Rule
    public EmbeddedTestServerRule mTestServerRule = new EmbeddedTestServerRule();

    private static final String[] FAKE_MOST_VISITED_URLS = new String[] {
            "/chrome/test/data/android/navigate/one.html",
            "/chrome/test/data/android/navigate/two.html",
            "/chrome/test/data/android/navigate/three.html",
            "/chrome/test/data/android/navigate/four.html",
            "/chrome/test/data/android/navigate/five.html",
            "/chrome/test/data/android/navigate/six.html",
            "/chrome/test/data/android/navigate/seven.html",
            "/chrome/test/data/android/navigate/eight.html",
            "/chrome/test/data/android/navigate/nine.html",
    };

    private NewTabPage mNtp;
    private FakeMostVisitedSites mMostVisitedSites;

    @Before
    public void setUp() throws Exception {
        String[] siteSuggestionUrls = mTestServerRule.getServer().getURLs(FAKE_MOST_VISITED_URLS);

        mMostVisitedSites = new FakeMostVisitedSites();
        mSuggestionsDeps.getFactory().mostVisitedSites = mMostVisitedSites;

        mMostVisitedSites.setTileSuggestions(siteSuggestionUrls);

        mSuggestionsDeps.getFactory().suggestionsSource = new FakeSuggestionsSource();

    }

    @Test
    @SmallTest
    @Feature({"NewTabPage"})
    public void testHomePageIsMovedToFirstRowWhenNotThereInitially() throws Exception {
        mMostVisitedSites.changeSiteSourceAtIndex(7, TileSource.HOMEPAGE);
        startActivity();

        TileView homePageTileView = findHomePageTileView();

        Assert.assertNotNull(homePageTileView);

        ViewGroup.MarginLayoutParams marginLayoutParams =
                (ViewGroup.MarginLayoutParams) homePageTileView.getLayoutParams();

        // The tiles on the top row have margin equal to zero.
        Assert.assertEquals(marginLayoutParams.topMargin, 0);
    }

    @Test
    @SmallTest
    @Feature({"NewTabPage"})
    public void testHomePageStaysAtFirstRowWhenThereInitially() throws Exception {
        mMostVisitedSites.changeSiteSourceAtIndex(2, TileSource.HOMEPAGE);
        startActivity();

        TileView homePageTileView = findHomePageTileView();

        Assert.assertNotNull(homePageTileView);

        ViewGroup.MarginLayoutParams marginLayoutParams =
                (ViewGroup.MarginLayoutParams) homePageTileView.getLayoutParams();

        // The tiles on the top row have margin equal to zero.
        Assert.assertEquals(marginLayoutParams.topMargin, 0);
    }

    @Nullable
    private TileView findHomePageTileView() {
        TileGridLayout tileGridLayout = getTileGridLayout();

        TileView homePageTileView = null;

        for (int i = 0; i < tileGridLayout.getChildCount(); i++) {
            TileView view = (TileView) tileGridLayout.getChildAt(i);
            if (view.getTileSource() == TileSource.HOMEPAGE) {
                homePageTileView = view;
                break;
            }
        }
        return homePageTileView;
    }

    private void startActivity() throws InterruptedException {
        mActivityTestRule.startMainActivityWithURL(UrlConstants.NTP_URL);
        Tab mTab = mActivityTestRule.getActivity().getActivityTab();
        NewTabPageTestUtils.waitForNtpLoaded(mTab);

        Assert.assertTrue(mTab.getNativePage() instanceof NewTabPage);
        mNtp = (NewTabPage) mTab.getNativePage();

        RecyclerViewTestUtils.waitForStableRecyclerView(getRecyclerView());
    }

    private NewTabPageRecyclerView getRecyclerView() {
        return mNtp.getNewTabPageView().getRecyclerView();
    }

    private TileGridLayout getTileGridLayout() {
        TileGridLayout tileGridLayout =
                mNtp.getNewTabPageView().findViewById(R.id.tile_grid_layout);
        Assert.assertNotNull("Unable to retrieve the TileGridLayout.", tileGridLayout);
        return tileGridLayout;
    }
}
