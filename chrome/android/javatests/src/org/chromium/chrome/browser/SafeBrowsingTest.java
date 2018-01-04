// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.safe_browsing.SafeBrowsingApiBridge;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content_public.browser.WebContents;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.concurrent.Callable;

/**
 * Test integration with the SafeBrowsingApiHandler.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public final class SafeBrowsingTest {
    private void waitForInterstitial(final boolean shouldBeShown) {
        CriteriaHelper.pollUiThread(Criteria.equals(shouldBeShown, new Callable<Boolean>() {
            @Override
            public Boolean call() {
                return getWebContents().isShowingInterstitialPage();
            }
        }));
    }

    private WebContents getWebContents() {
        return mActivityTestRule.getActivity().getCurrentContentViewCore().getWebContents();
    }

    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);
    private EmbeddedTestServer mTestServer;

    @Before
    public void setUp() throws Exception {
        // Create a new instance to ensure the Class is loaded.
        SafeBrowsingApiBridge.setSafeBrowsingHandlerType(
                new MockSafeBrowsingApiHandler().getClass());
        mTestServer = EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    @After
    public void tearDown() throws Exception {
        mTestServer.stopAndDestroyServer();
    }

    @Test
    @MediumTest
    public void interstitialPage() throws Exception {
        String url = mTestServer.getURL("/chrome/test/data/android/about.html");
        mActivityTestRule.loadUrl(url);
        waitForInterstitial(false);

        // Add |url| to the blacklist.
        MockSafeBrowsingApiHandler.addMockResponseForTesting(
                url, "{\"matches\":[{\"threat_type\":\"5\"}]}");
        mActivityTestRule.loadUrl(url);
        waitForInterstitial(true);
    }
}
