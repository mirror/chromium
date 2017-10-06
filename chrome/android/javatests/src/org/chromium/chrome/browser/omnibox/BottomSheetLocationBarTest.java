// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import android.annotation.SuppressLint;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.parameter.CommandLineParameter;
import org.chromium.base.test.util.parameter.SkipCommandLineParameterization;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabObserver;
import org.chromium.chrome.test.BottomSheetTestRule;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.net.test.EmbeddedTestServer;
import org.chromium.net.test.ServerCertificate;
import org.chromium.ui.test.util.UiRestriction;

/**
 * Tests for {@link LocationBarLayout} UI component in the home sheet.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG})
@CommandLineParameter({"", "disable-features=" + ChromeFeatureList.SPANNABLE_INLINE_AUTOCOMPLETE})
@Restriction(UiRestriction.RESTRICTION_TYPE_PHONE) // ChromeHome is only enabled on phones
@SuppressLint("SetTextI18n")
public class BottomSheetLocationBarTest {
    @Rule
    public BottomSheetTestRule mBottomSheetTestRule = new BottomSheetTestRule();

    private static final String THEME_COLOR_TEST_PAGE =
            "/chrome/test/data/android/theme_color_test.html";

    private final CallbackHelper mDidThemeColorChangedCallbackHelper = new CallbackHelper();

    @Before
    public void setUp() throws Exception {
        mBottomSheetTestRule.startMainActivityOnBlankPage();
        new TabModelSelectorTabObserver(mBottomSheetTestRule.getActivity().getTabModelSelector()) {
            @Override
            public void onDidChangeThemeColor(Tab tab, int color) {
                mDidThemeColorChangedCallbackHelper.notifyCalled();
            }
        };
    }

    /**
     * Test whether the color of the Location bar is correct for HTTPS scheme.
     */
    @Test
    @SmallTest
    @SkipCommandLineParameterization
    public void testHttpsLocationBarColor() throws Exception {
        EmbeddedTestServer mTestServer = EmbeddedTestServer.createAndStartHTTPSServer(
                InstrumentationRegistry.getInstrumentation().getContext(),
                ServerCertificate.CERT_OK);
        try {
            final String testHttpsUrl = mTestServer.getURL(THEME_COLOR_TEST_PAGE);

            mBottomSheetTestRule.loadUrl(testHttpsUrl);
            mDidThemeColorChangedCallbackHelper.waitForCallback(0);
            LocationBarLayout locationBarLayout =
                    (LocationBarLayout) mBottomSheetTestRule.getActivity().findViewById(
                            R.id.location_bar);
            Assert.assertTrue(locationBarLayout.shouldEmphasizeHttpsScheme());
        } finally {
            mTestServer.stopAndDestroyServer();
        }
    }
}
