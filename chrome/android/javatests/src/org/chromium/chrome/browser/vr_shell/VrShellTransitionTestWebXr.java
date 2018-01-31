// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import static org.chromium.chrome.browser.vr_shell.VrTestFramework.PAGE_LOAD_TIMEOUT_S;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_LONG_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_SHORT_MS;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_VIEWER_DAYDREAM;

import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.vr_shell.rules.ChromeTabbedActivityVrTestRule;
import org.chromium.chrome.browser.vr_shell.util.XrTransitionUtils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.util.concurrent.TimeoutException;

/**
 * End-to-end tests for state transitions in VR, e.g. exiting WebXR presentation
 * into the VR browser. This is a temporary class for testing transitions with WebXR - it will be
 * merged with VrShellTransitionTest once WebVR is replaced with WebXR.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class VrShellTransitionTestWebXr {
    // We explicitly instantiate a rule here instead of using parameterization since this class
    // only ever runs in ChromeTabbedActivity.
    @Rule
    public ChromeTabbedActivityVrTestRule mVrTestRule = new ChromeTabbedActivityVrTestRule();

    private XrTestFramework mXrTestFramework;

    @Before
    public void setUp() throws Exception {
        mXrTestFramework = new XrTestFramework(mVrTestRule);
    }

    /**
     * Tests that the reported display dimensions are correct when exiting
     * from WebVR presentation to the VR browser.
     */
    @Test
    @CommandLineFlags.Add("enable-features=WebXR")
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @MediumTest
    public void testExitPresentationWebXrToVrShell()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        XrTransitionUtils.forceEnterVr();
        XrTransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        mXrTestFramework.loadUrlAndAwaitInitialization(
                XrTestFramework.getHtmlTestFile("test_navigation_webxr_page"), PAGE_LOAD_TIMEOUT_S);
        VrShellImpl vrShellImpl = (VrShellImpl) TestVrShellDelegate.getVrShellForTesting();
        float expectedWidth = vrShellImpl.getContentWidthForTesting();
        float expectedHeight = vrShellImpl.getContentHeightForTesting();
        XrTransitionUtils.enterPresentationOrFail(mXrTestFramework.getFirstTabCvc());

        // Validate our size is what we expect while in VR.
        // We aren't comparing for equality because there is some rounding that occurs.
        String javascript = "Math.abs(screen.width - " + expectedWidth + ") <= 1 && "
                + "Math.abs(screen.height - " + expectedHeight + ") <= 1";
        Assert.assertTrue(XrTestFramework.pollJavaScriptBoolean(
                javascript, POLL_TIMEOUT_LONG_MS, mXrTestFramework.getFirstTabWebContents()));

        // Exit presentation through JavaScript.
        XrTestFramework.runJavaScriptOrFail("exclusiveSession.end();", POLL_TIMEOUT_SHORT_MS,
                mXrTestFramework.getFirstTabWebContents());

        Assert.assertTrue(XrTestFramework.pollJavaScriptBoolean(
                javascript, POLL_TIMEOUT_LONG_MS, mXrTestFramework.getFirstTabWebContents()));
    }

    /**
     * Tests that entering WebVR presentation from the VR browser, exiting presentation, and
     * re-entering presentation works. This is a regression test for crbug.com/799999.
     */
    @Test
    @CommandLineFlags.Add("enable-features=WebXR")
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @MediumTest
    public void testWebXrReEntryFromVrBrowser() throws InterruptedException, TimeoutException {
        XrTransitionUtils.forceEnterVr();
        XrTransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        EmulatedVrController controller = new EmulatedVrController(mVrTestRule.getActivity());

        mXrTestFramework.loadUrlAndAwaitInitialization(
                XrTestFramework.getHtmlTestFile("test_webxr_reentry_from_vr_browser"),
                PAGE_LOAD_TIMEOUT_S);
        XrTransitionUtils.enterPresentationOrFail(mXrTestFramework.getFirstTabCvc());

        XrTestFramework.executeStepAndWait(
                "stepVerifyFirstPresent()", mXrTestFramework.getFirstTabWebContents());
        // The bug did not reproduce with vrDisplay.exitPresent(), so it might not reproduce with
        // session.end(). Instead, use the controller to exit.
        controller.pressReleaseAppButton();
        XrTestFramework.executeStepAndWait(
                "stepVerifyMagicWindow()", mXrTestFramework.getFirstTabWebContents());

        XrTransitionUtils.enterPresentationOrFail(mXrTestFramework.getFirstTabCvc());
        XrTestFramework.executeStepAndWait(
                "stepVerifySecondPresent()", mXrTestFramework.getFirstTabWebContents());

        XrTestFramework.endTest(mXrTestFramework.getFirstTabWebContents());
    }
}
