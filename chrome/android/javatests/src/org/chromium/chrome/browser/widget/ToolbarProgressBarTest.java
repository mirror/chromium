// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.animation.Animator;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.view.View;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.util.MathUtils;
import org.chromium.chrome.browser.widget.ClipDrawableProgressBar.ProgressBarObserver;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.test.util.JavaScriptUtils;
import org.chromium.content.browser.test.util.TestCallbackHelperContainer.OnPageFinishedHelper;
import org.chromium.content.browser.test.util.TestCallbackHelperContainer.OnPageStartedHelper;
import org.chromium.content.browser.test.util.TestWebContentsObserver;
import org.chromium.content_public.browser.WebContents;
import org.chromium.net.test.EmbeddedTestServer;
import org.chromium.ui.test.util.UiRestriction;

import java.util.concurrent.TimeoutException;

/**
 * Tests related to the ToolbarProgressBar.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG})
public class ToolbarProgressBarTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    private static final String TEST_PAGE = "/chrome/test/data/android/progressbar_test.html";

    private final CallbackHelper mProgressUpdateHelper = new CallbackHelper();
    private final CallbackHelper mProgressVisibilityHelper = new CallbackHelper();
    private ToolbarProgressBar mProgressBar;

    @Before
    public void setUp() throws InterruptedException, TimeoutException {
        mActivityTestRule.startMainActivityOnBlankPage();
        mProgressBar = mActivityTestRule.getActivity()
                               .getToolbarManager()
                               .getToolbarLayout()
                               .getProgressBar();

        mProgressBar.resetStartCountForTesting();

        mProgressBar.setProgressBarObserver(new ProgressBarObserver() {
            @Override
            public void onVisibleProgressUpdated() {
                mProgressUpdateHelper.notifyCalled();
            }

            @Override
            public void onVisibilityChanged() {
                mProgressVisibilityHelper.notifyCalled();
            }
        });

        // Make sure the progress bar is invisible before starting any of the tests.
        if (mProgressBar.getVisibility() == View.VISIBLE) {
            int count = mProgressVisibilityHelper.getCallCount();
            mProgressVisibilityHelper.waitForCallback(count, 1);
        }
    }

    /**
     * Test that the progress bar only traverses the page a single time per navigation.
     */
    @Test
    @Feature({"Android-Progress-Bar"})
    @MediumTest
    @Restriction(UiRestriction.RESTRICTION_TYPE_PHONE)
    public void testProgressBarTraversesScreenOnce() throws InterruptedException, TimeoutException {
        EmbeddedTestServer testServer =
                EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());

        final WebContents webContents =
                mActivityTestRule.getActivity().getActivityTab().getWebContents();

        TestWebContentsObserver observer = new TestWebContentsObserver(webContents);
        // Start and stop load events are carefully tracked; there should be two start-stop pairs
        // that do not overlap.
        OnPageStartedHelper startHelper = observer.getOnPageStartedHelper();
        OnPageFinishedHelper finishHelper = observer.getOnPageFinishedHelper();

        // Ensure no load events have occurred yet.
        assertEquals("Page load should not have started.", 0, startHelper.getCallCount());
        assertEquals("Page load should not have finished.", 0, finishHelper.getCallCount());

        mActivityTestRule.loadUrl(testServer.getURL(TEST_PAGE));

        // Wait for the initial page to be loaded if it hasn't already.
        if (finishHelper.getCallCount() == 0) {
            finishHelper.waitForCallback(finishHelper.getCallCount(), 1);
        }

        // Exactly one start load and one finish load event should have occurred.
        assertEquals("Page load should have started.", 1, startHelper.getCallCount());
        assertEquals("Page load should have finished.", 1, finishHelper.getCallCount());

        // Load content in the iframe of the test page to trigger another load.
        JavaScriptUtils.executeJavaScript(webContents, "loadIframeInPage();");

        // A load start will be triggered.
        startHelper.waitForCallback(startHelper.getCallCount(), 1);
        assertEquals("Iframe should have triggered page load.", 2, startHelper.getCallCount());

        // Wait for the iframe to finish loading.
        finishHelper.waitForCallback(finishHelper.getCallCount(), 1);
        assertEquals("Iframe should have finished loading.", 2, finishHelper.getCallCount());

        // Though the page triggered two load events, the progress bar should have only appeared a
        // single time.
        assertEquals("The progress bar should have only started once.", 1,
                mProgressBar.getStartCountForTesting());
    }

    /**
     * Test that the progress bar indeterminate animation completely traverses the screen.
     */
    @Test
    @Feature({"Android-Progress-Bar"})
    @MediumTest
    @Restriction(UiRestriction.RESTRICTION_TYPE_PHONE)
    public void testProgressBarCompletion_indeterminateAnimation()
            throws InterruptedException, TimeoutException {
        Animator progressAnimator = mProgressBar.getIndeterminateAnimatorForTesting();

        int currentVisibilityCallCount = mProgressVisibilityHelper.getCallCount();

        ThreadUtils.runOnUiThreadBlocking(() -> mProgressBar.start());
        assertFalse("Indeterminate animation should not be running.", progressAnimator.isRunning());

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mProgressBar.startIndeterminateAnimationForTesting();
            mProgressBar.setProgress(0.5f);
        });

        // Wait for a visibility change.
        mProgressVisibilityHelper.waitForCallback(currentVisibilityCallCount, 1);
        currentVisibilityCallCount++;

        assertTrue("Indeterminate animation should be running.", progressAnimator.isRunning());

        // Wait for progress updates to reach 50%.
        int currentProgressCallCount = mProgressUpdateHelper.getCallCount();
        while (!MathUtils.areFloatsEqual(mProgressBar.getProgress(), 0.5f)) {
            mProgressUpdateHelper.waitForCallback(currentProgressCallCount, 1);
            currentProgressCallCount++;
        }

        ThreadUtils.runOnUiThreadBlocking(() -> mProgressBar.finish(true));

        // Wait for progress updates to reach 100%.
        currentProgressCallCount = mProgressUpdateHelper.getCallCount();
        while (!MathUtils.areFloatsEqual(mProgressBar.getProgress(), 1.0f)) {
            mProgressUpdateHelper.waitForCallback(currentProgressCallCount, 1);
            currentProgressCallCount++;
        }

        // Make sure the progress bar remains visible through completion.
        assertEquals("Progress bar should still be visible.", mProgressBar.getVisibility(),
                View.VISIBLE);

        assertEquals("Progress should have reached 100%.", mProgressBar.getProgress(), 1.0f,
                MathUtils.EPSILON);

        // Wait for a visibility change now that progress has completed.
        mProgressVisibilityHelper.waitForCallback(currentVisibilityCallCount, 1);

        assertFalse("Indeterminate animation should not be running.", progressAnimator.isRunning());
        assertEquals("Progress bar should not be visible.", mProgressBar.getVisibility(),
                View.INVISIBLE);
    }

    /**
     * Test that the progress bar completely traverses the screen without animation.
     */
    @Test
    @Feature({"Android-Progress-Bar"})
    @MediumTest
    @Restriction(UiRestriction.RESTRICTION_TYPE_PHONE)
    public void testProgressBarCompletion_noAnimation()
            throws InterruptedException, TimeoutException {
        int currentVisibilityCallCount = mProgressVisibilityHelper.getCallCount();
        int currentProgressCallCount = mProgressUpdateHelper.getCallCount();

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mProgressBar.start();
            mProgressBar.setProgress(0.5f);
        });

        // Wait for a visibility change.
        mProgressVisibilityHelper.waitForCallback(currentVisibilityCallCount, 1);
        currentVisibilityCallCount++;

        // Wait for progress updates to reach 50%.
        mProgressUpdateHelper.waitForCallback(currentProgressCallCount, 1);
        currentProgressCallCount++;
        assertEquals("Progress should have reached 50%.", mProgressBar.getProgress(), 0.5f,
                MathUtils.EPSILON);

        currentProgressCallCount = mProgressUpdateHelper.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(() -> mProgressBar.finish(true));

        // Wait for progress updates to reach 100%.
        mProgressUpdateHelper.waitForCallback(currentProgressCallCount, 1);
        currentProgressCallCount++;
        assertEquals("Progress should have reached 100%.", mProgressBar.getProgress(), 1.0f,
                MathUtils.EPSILON);

        // Make sure the progress bar remains visible through completion.
        assertEquals("Progress bar should still be visible.", mProgressBar.getVisibility(),
                View.VISIBLE);

        // Wait for a visibility change now that progress has completed.
        mProgressVisibilityHelper.waitForCallback(currentVisibilityCallCount, 1);

        assertEquals("Progress bar should not be visible.", mProgressBar.getVisibility(),
                View.INVISIBLE);
    }

    /**
     * Test that the progress bar ends immediately if #finish(...) is called with delay = false.
     */
    @Test
    @Feature({"Android-Progress-Bar"})
    @MediumTest
    @Restriction(UiRestriction.RESTRICTION_TYPE_PHONE)
    public void testProgressBarCompletion_indeterminateAnimation_noDelay()
            throws InterruptedException, TimeoutException {
        Animator progressAnimator = mProgressBar.getIndeterminateAnimatorForTesting();

        int currentVisibilityCallCount = mProgressVisibilityHelper.getCallCount();

        ThreadUtils.runOnUiThreadBlocking(() -> mProgressBar.start());
        assertFalse("Indeterminate animation should not be running.", progressAnimator.isRunning());

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mProgressBar.startIndeterminateAnimationForTesting();
            mProgressBar.setProgress(0.5f);
        });

        // Wait for a visibility change.
        mProgressVisibilityHelper.waitForCallback(currentVisibilityCallCount, 1);
        currentVisibilityCallCount++;

        assertTrue("Indeterminate animation should be running.", progressAnimator.isRunning());

        // Wait for progress updates to reach 50%.
        int currentProgressCallCount = mProgressUpdateHelper.getCallCount();
        while (!MathUtils.areFloatsEqual(mProgressBar.getProgress(), 0.5f)) {
            mProgressUpdateHelper.waitForCallback(currentProgressCallCount, 1);
            currentProgressCallCount++;
        }

        // Finish progress with no delay.
        ThreadUtils.runOnUiThreadBlocking(() -> mProgressBar.finish(false));

        // The progress bar should immediately be invisible.
        assertEquals("Progress bar should still be visible.", mProgressBar.getVisibility(),
                View.INVISIBLE);
        assertFalse("Indeterminate animation should not be running.", progressAnimator.isRunning());
    }

    /**
     * Test that the progress bar resets if a navigation occurs mid-progress while the indeterminate
     * animation is running.
     */
    @Test
    @Feature({"Android-Progress-Bar"})
    @MediumTest
    @Restriction(UiRestriction.RESTRICTION_TYPE_PHONE)
    public void testProgressBarReset_indeterminateAnimation()
            throws InterruptedException, TimeoutException {
        Animator progressAnimator = mProgressBar.getIndeterminateAnimatorForTesting();

        int currentVisibilityCallCount = mProgressVisibilityHelper.getCallCount();

        ThreadUtils.runOnUiThreadBlocking(() -> mProgressBar.start());
        assertFalse("Indeterminate animation should not be running.", progressAnimator.isRunning());

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mProgressBar.startIndeterminateAnimationForTesting();
            mProgressBar.setProgress(0.5f);
        });

        // Wait for a visibility change.
        mProgressVisibilityHelper.waitForCallback(currentVisibilityCallCount, 1);
        currentVisibilityCallCount++;

        assertTrue("Indeterminate animation should be running.", progressAnimator.isRunning());

        // Wait for progress updates to reach 50%.
        int currentProgressCallCount = mProgressUpdateHelper.getCallCount();
        while (!MathUtils.areFloatsEqual(mProgressBar.getProgress(), 0.5f)) {
            mProgressUpdateHelper.waitForCallback(currentProgressCallCount, 1);
            currentProgressCallCount++;
        }

        // Restart the progress bar.
        currentProgressCallCount = mProgressUpdateHelper.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(() -> mProgressBar.start());

        // Wait for progress update.
        mProgressUpdateHelper.waitForCallback(currentProgressCallCount, 1);
        currentProgressCallCount++;

        // Make sure the progress bar remains visible through completion.
        assertEquals("Progress bar should still be visible.", mProgressBar.getVisibility(),
                View.VISIBLE);

        assertEquals(
                "Progress should be at 0%.", mProgressBar.getProgress(), 0.0f, MathUtils.EPSILON);
    }
}
