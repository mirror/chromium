// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import static org.chromium.chrome.browser.vr_shell.VrTestFramework.PAGE_LOAD_TIMEOUT_S;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_LONG_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_SHORT_MS;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_DEVICE_DAYDREAM;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_VIEWER_DAYDREAM;

import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.vr_shell.rules.ChromeTabbedActivityVrTestRule;
import org.chromium.chrome.browser.vr_shell.util.VrInfoBarUtils;
import org.chromium.chrome.browser.vr_shell.util.XrTransitionUtils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.ChromeTabUtils;

import java.util.concurrent.TimeoutException;

/**
 * Tests for the infobar that prompts the user to enter feedback on their VR browsing experience.
 * This is a temporary class for testing the feedback info bar with WebXR - it will be merged with
 * VrFeedbackInfoBarTest once WebVR is replaced with WebXR.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE, "enable-features=WebXR"})
@Restriction(RESTRICTION_TYPE_DEVICE_DAYDREAM)
@RetryOnFailure(message = "These tests have a habit of hitting a race condition, preventing "
                + "VR entry. See crbug.com/762724")
public class VrFeedbackInfoBarTestWebXr {
    // We explicitly instantiate a rule here instead of using parameterization since this class
    // only ever runs in ChromeTabbedActivity.
    @Rule
    public ChromeTabbedActivityVrTestRule mVrTestRule = new ChromeTabbedActivityVrTestRule();

    private XrTestFramework mXrTestFramework;

    private static final String TEST_PAGE_2D_URL =
            XrTestFramework.getHtmlTestFile("test_navigation_2d_page");
    private static final String TEST_PAGE_WEBXR_URL =
            XrTestFramework.getHtmlTestFile("generic_webxr_page");

    @Before
    public void setUp() throws Exception {
        mXrTestFramework = new XrTestFramework(mVrTestRule);
        Assert.assertFalse(VrFeedbackStatus.getFeedbackOptOut());
    }

    private void assertState(boolean isInVr, boolean isInfobarVisible) {
        Assert.assertEquals("Browser is in VR", isInVr, VrShellDelegate.isInVr());
        VrInfoBarUtils.expectInfoBarPresent(mXrTestFramework, isInfobarVisible);
    }

    private void enterThenExitVr() {
        XrTransitionUtils.forceEnterVr();
        XrTransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        assertState(true /* isInVr */, false /* isInfobarVisible  */);
        XrTransitionUtils.forceExitVr();
    }

    /**
     * Tests that we only show the feedback prompt when the user has actually used the VR browser.
     */
    @Test
    @MediumTest
    public void testFeedbackOnlyOnVrBrowsing() throws InterruptedException, TimeoutException {
        // Enter VR presentation mode.
        mXrTestFramework.loadUrlAndAwaitInitialization(TEST_PAGE_WEBXR_URL, PAGE_LOAD_TIMEOUT_S);
        XrTransitionUtils.enterPresentationOrFail(mXrTestFramework.getFirstTabCvc());
        assertState(true /* isInVr */, false /* isInfobarVisible  */);
        Assert.assertTrue(TestVrShellDelegate.getVrShellForTesting().getWebVrModeEnabled());

        // Exiting VR should not prompt for feedback since the no VR browsing was performed.
        XrTransitionUtils.forceExitVr();
        assertState(false /* isInVr */, false /* isInfobarVisible  */);
    }

    /**
     * Tests that we show the prompt if the VR browser is used after exiting a WebXR exclusive
     * session.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    public void testExitPresentationInVr() throws InterruptedException, TimeoutException {
        // Enter VR presentation mode.
        mXrTestFramework.loadUrlAndAwaitInitialization(TEST_PAGE_WEBXR_URL, PAGE_LOAD_TIMEOUT_S);
        XrTransitionUtils.enterPresentationOrFail(mXrTestFramework.getFirstTabCvc());
        assertState(true /* isInVr */, false /* isInfobarVisible  */);
        Assert.assertTrue(TestVrShellDelegate.getVrShellForTesting().getWebVrModeEnabled());

        // Exit presentation mode by navigating to a different url.
        ChromeTabUtils.waitForTabPageLoaded(
                mVrTestRule.getActivity().getActivityTab(), new Runnable() {
                    @Override
                    public void run() {
                        XrTestFramework.runJavaScriptOrFail(
                                "window.location.href = '" + TEST_PAGE_2D_URL + "';",
                                POLL_TIMEOUT_SHORT_MS, mXrTestFramework.getFirstTabWebContents());
                    }
                }, POLL_TIMEOUT_LONG_MS);

        // Exiting VR should prompt for feedback since 2D browsing was performed after.
        XrTransitionUtils.forceExitVr();
        assertState(false /* isInVr */, true /* isInfobarVisible  */);
    }
}
