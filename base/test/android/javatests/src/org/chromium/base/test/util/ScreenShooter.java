// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.util;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.app.Instrumentation;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.UiDevice;

import org.junit.rules.TestWatcher;
import org.junit.runner.Description;

import java.io.File;

/**
 * Rule for taking screen shots within tests. Screenshots are saved as
 * UiCapture/<test class>/<test name>/<shot name>.png
 *
 * A simple example:
 * <pre>
 * {@code
 *
 * @RunWith(ChromeJUnit4ClassRunner.class)
 * @CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
 * public class ExampleUiCaptureTest {
 *    @Rule
 *    public ChromeActivityTestRule<ChromeTabbedActivity> mActivityTestRule =
 *            new ChromeActivityTestRule<>(ChromeTabbedActivity.class);
 *
 *    @Rule
 *    public ScreenShooter mScreenShooter = new ScreenShooter();
 *
 *    @Before
 *    public void setUp() throws InterruptedException {
 *        mActivityTestRule.startMainActivityFromLauncher();
 *    }
 *
 *    // Capture the New Tab Page and the tab switcher.
 *    @Test
 *    @SmallTest
 *    public void testCaptureTabSwitcher() throws IOException, InterruptedException {
 *        mScreenShooter.shoot("NTP");
 *        ChromeTabbedActivity activity = mActivityTestRule.getActivity();
 *        OverviewModeBehaviorWatcher overviewModeWatcher =
 *                new OverviewModeBehaviorWatcher(activity.getLayoutManager(), true, false);
 *        View tabSwitcherButton = activity.findViewById(R.id.tab_switcher_button);
 *        TouchCommon.singleClickView(tabSwitcherButton);
 *        overviewModeWatcher.waitForBehavior();
 *        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
 *        mScreenShooter.shoot("Tab_switcher");
 *    }
 * }
 * }
 * </pre>
 */
public class ScreenShooter extends TestWatcher {
    private Instrumentation mInstrumentation;
    private UiDevice mDevice;
    private File mDir;

    /**
     *
     */
    public ScreenShooter() {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mDevice = UiDevice.getInstance(mInstrumentation);
    }

    @Override
    protected void starting(Description d) {
        mDir = new File(UrlUtils.getIsolatedTestFilePath(
                "UiCapture/" + d.getClassName() + "/" + d.getMethodName()));
        mDir.mkdirs();
    }

    /**
     * Take a screen shot and save it to a file.
     *
     * @param shotName The name of this particular screenshot within this test. This will be used to
     * name the image file.
     */
    public void shoot(String shotName) {
        assertNotNull("ScreenShooter rule initialized", mDir);
        assertTrue("Screenshot " + shotName,
                mDevice.takeScreenshot(new File(mDir, shotName + ".png")));
    }
}
