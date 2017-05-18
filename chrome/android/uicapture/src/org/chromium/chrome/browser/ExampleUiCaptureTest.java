// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.view.View;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.ScreenShooter;
import org.chromium.chrome.R;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.OverviewModeBehaviorWatcher;
import org.chromium.content.browser.test.util.TouchCommon;

import java.io.IOException;

/**
 * Simple test to demonstrate use of ScreenShooter rule.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class ExampleUiCaptureTest {
    @Rule
    public ChromeActivityTestRule<ChromeTabbedActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeTabbedActivity.class);

    @Rule
    public ScreenShooter mScreenShooter = new ScreenShooter();

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityFromLauncher();
    }

    /**
     * Capture the New Tab Page and the tab switcher.
     * @throws IOException
     * @throws InterruptedException
     */
    @Test
    @SmallTest
    public void testCaptureTabSwitcher() throws IOException, InterruptedException {
        mScreenShooter.shoot("NTP");
        ChromeTabbedActivity activity = mActivityTestRule.getActivity();
        OverviewModeBehaviorWatcher overviewModeWatcher =
                new OverviewModeBehaviorWatcher(activity.getLayoutManager(), true, false);
        View tabSwitcherButton = activity.findViewById(R.id.tab_switcher_button);
        TouchCommon.singleClickView(tabSwitcherButton);
        overviewModeWatcher.waitForBehavior();
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        mScreenShooter.shoot("Tab_switcher");
    }
}
