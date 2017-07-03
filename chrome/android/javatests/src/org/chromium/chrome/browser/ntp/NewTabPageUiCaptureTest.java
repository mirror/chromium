// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.uiautomator.UiDevice;
import android.view.View;

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
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.ntp.cards.NewTabPageRecyclerView;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.NewTabPageTestUtils;
import org.chromium.chrome.test.util.browser.suggestions.DummySuggestionsEventReporter;
import org.chromium.chrome.test.util.browser.suggestions.SuggestionsDependenciesRule;
import org.chromium.net.test.EmbeddedTestServerRule;

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
    public EmbeddedTestServerRule mTestServerRule = new EmbeddedTestServerRule();

    @Rule
    public SuggestionsDependenciesRule mSuggestionsRule = new SuggestionsDependenciesRule();

    private static final int MAX_WINDOW_UPDATE_TIME_MS = 1000;

    private NewTabPage mNtp;

    @Before
    public void setUp() throws Exception {
        final SuggestionsDependenciesRule.TestFactory factory = mSuggestionsRule.getFactory();
        factory.suggestionsSource = NtpUiCaptureTestData.getSuggestionsSource();
        factory.eventReporter = new DummySuggestionsEventReporter();
        factory.largeIconBridge = NtpUiCaptureTestData.getLargeIconBridge();
        factory.mostVisitedSites = NtpUiCaptureTestData.getMostVisitedSites();

        mActivityTestRule.startMainActivityWithURL(UrlConstants.NTP_URL);
        Tab tab = mActivityTestRule.getActivity().getActivityTab();
        NewTabPageTestUtils.waitForNtpLoaded(tab);

        Assert.assertTrue(tab.getNativePage() instanceof NewTabPage);
        mNtp = (NewTabPage) tab.getNativePage();
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