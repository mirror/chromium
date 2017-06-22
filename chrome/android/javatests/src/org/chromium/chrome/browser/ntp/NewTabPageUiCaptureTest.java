// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.uiautomator.UiDevice;
import android.view.View;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.base.test.util.ScreenShooter;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.favicon.LargeIconBridge;
import org.chromium.chrome.browser.ntp.cards.NewTabPageRecyclerView;
import org.chromium.chrome.browser.suggestions.FakeMostVisitedSites;
import org.chromium.chrome.browser.suggestions.SuggestionsUiDelegateImpl;
import org.chromium.chrome.browser.suggestions.TileGroupDelegateImpl;
import org.chromium.chrome.browser.suggestions.TileSource;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.UiCaptureSuggestionsRule;
import org.chromium.chrome.test.util.NewTabPageTestUtils;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.HashMap;
import java.util.Map;

/**
 * Capture the New Tab Page UI for UX review.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({
        ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG,
})
@RetryOnFailure
public class NewTabPageUiCaptureTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    @Rule
    public ScreenShooter mScreenShooter = new ScreenShooter();

    @Rule
    public EmbeddedTestServer mTestServer = new EmbeddedTestServer();

    @Rule
    public UiCaptureSuggestionsRule mSuggestionsRule = new UiCaptureSuggestionsRule();

    private static final int MAX_WINDOW_UPDATE_TIME_MS = 1000;

    private static final String[] FAKE_MOST_VISITED_TITLES =
            new String[] {"Simple", "A longer label", "A label that should be truncated",
                    "Dummy site", "Fake", "Not real", "Test", "Most visited 8"};
    private static final String[] FAKE_MOST_VISITED_WHITELIST_ICON_PATHS =
            new String[] {"", "", "", "", "", "", "", ""};
    private static final int[] FAKE_MOST_VISITED_SOURCES = new int[] {TileSource.TOP_SITES,
            TileSource.TOP_SITES, TileSource.TOP_SITES, TileSource.TOP_SITES, TileSource.TOP_SITES,
            TileSource.TOP_SITES, TileSource.TOP_SITES, TileSource.TOP_SITES};
    private static final String[] FAKE_MOST_VISITED_URLS = new String[] {
            "junk1", "junk2", "junk3", "Dummy", "Fake", "Not real", "Test", "Most visited 8"};
    private static final int[] FAKE_MOST_VISITED_COLORS = {Color.RED, Color.BLUE, Color.GREEN,
            Color.BLACK, Color.CYAN, Color.DKGRAY, Color.BLUE, Color.YELLOW};

    private NewTabPage mNtp;

    @Before
    public void setUp() throws Exception {
        FakeMostVisitedSites mostVisitedSites = new FakeMostVisitedSites();
        mostVisitedSites.setTileSuggestions(
                new String[0], new String[0], new String[0], new int[0]);
        TileGroupDelegateImpl.setMostVisitedSitesForTests(mostVisitedSites);

        mActivityTestRule.startMainActivityWithURL(UrlConstants.NTP_URL);
        Tab tab = mActivityTestRule.getActivity().getActivityTab();
        NewTabPageTestUtils.waitForNtpLoaded(tab);

        Assert.assertTrue(tab.getNativePage() instanceof NewTabPage);
        mNtp = (NewTabPage) tab.getNativePage();
        final Map<String, Bitmap> iconMap = new HashMap<>();
        iconMap.put(FAKE_MOST_VISITED_URLS[0],
                BitmapFactory.decodeFile(UrlUtils.getTestFilePath("/android/google.png")));
        iconMap.put(FAKE_MOST_VISITED_URLS[1],
                BitmapFactory.decodeResource(mActivityTestRule.getActivity().getResources(),
                        org.chromium.chrome.R.drawable.star_green));

        final Map<String, Integer> colorMap = new HashMap<>();
        for (int i = 0; i < FAKE_MOST_VISITED_URLS.length; i++) {
            colorMap.put(FAKE_MOST_VISITED_URLS[i], FAKE_MOST_VISITED_COLORS[i]);
        }

        NewTabPageView.NewTabPageManager manager = mNtp.getManagerForTesting();
        Assert.assertTrue(manager instanceof SuggestionsUiDelegateImpl);
        final SuggestionsUiDelegateImpl suggestionsDelegateImpl =
                (SuggestionsUiDelegateImpl) manager;
        suggestionsDelegateImpl.setLargeIconBridge(new LargeIconBridge(null) {
            @Override
            public boolean getLargeIconForUrl(
                    final String pageUrl, int desiredSizePx, final LargeIconCallback callback) {
                ThreadUtils.postOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        callback.onLargeIconAvailable(
                                iconMap.get(pageUrl), colorMap.get(pageUrl), true);
                    }
                });
                return true;
            }
        });
        mostVisitedSites.setTileSuggestions(FAKE_MOST_VISITED_TITLES, FAKE_MOST_VISITED_URLS,
                FAKE_MOST_VISITED_WHITELIST_ICON_PATHS, FAKE_MOST_VISITED_SOURCES);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mNtp.getNewTabPageView().getTileGroup().onSwitchToForeground();
            }
        });
    }

    @After
    public void tearDown() throws Exception {
        TileGroupDelegateImpl.setMostVisitedSitesForTests(null);
    }

    private void waitForWindowUpdates() {
        // Wait for update to start and finish.
        UiDevice device = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        device.waitForWindowUpdate(null, MAX_WINDOW_UPDATE_TIME_MS);
        device.waitForIdle(MAX_WINDOW_UPDATE_TIME_MS);
    }

    @Test
    @MediumTest
    @Feature({"NewTabPageTest", "UiCatalogue"})
    @ScreenShooter.Directory("New Tab Page")
    public void testCaptureNewTabPage() {
        waitForWindowUpdates();
        mScreenShooter.shoot("New Tab Page");
        // Scroll to search bar
        final NewTabPageRecyclerView recyclerView = mNtp.getNewTabPageView().getRecyclerView();

        final View fakebox = mNtp.getView().findViewById(org.chromium.chrome.R.id.search_box);
        final int scrollHeight = fakebox.getTop();

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                recyclerView.smoothScrollBy(0, scrollHeight);
            }
        });
        waitForWindowUpdates();
        mScreenShooter.shoot("New Tab Page scrolled");
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                recyclerView.smoothScrollBy(0, scrollHeight);
            }
        });
        waitForWindowUpdates();
        mScreenShooter.shoot("New Tab Page scrolled twice");
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                recyclerView.smoothScrollBy(0, scrollHeight);
            }
        });
        waitForWindowUpdates();
        mScreenShooter.shoot("New Tab Page scrolled thrice");
    }
}