// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.util;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.app.Instrumentation;
import android.content.res.Configuration;
import android.os.Build;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.UiDevice;

import org.junit.rules.TestWatcher;
import org.junit.runner.Description;

import java.io.File;

/**
 * Rule for taking screen shots within tests. Screenshots are saved as UiCapture/<test class>/<test
 * name>/<shot name>.png A simple example:
 *
 * <pre>
 * {
 *     &#64;code
 *
 *     &#64;RunWith(ChromeJUnit4ClassRunner.class)
 *     &#64;CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
 *     &#64;Restriction(RESTRICTION_TYPE_PHONE) // Tab switcher button only exists on phones.
 *     public class ExampleUiCaptureTest {
 *         &#64;Rule
 *         public ChromeActivityTestRule<ChromeTabbedActivity> mActivityTestRule =
 *                 new ChromeActivityTestRule<>(ChromeTabbedActivity.class);
 *
 *         &#64;Rule
 *         public ScreenShooter mScreenShooter = new ScreenShooter();
 *
 *         &#64;Before
 *         public void setUp() throws InterruptedException {
 *             mActivityTestRule.startMainActivityFromLauncher();
 *         }
 *
 *         // Capture the New Tab Page and the tab switcher.
 *         &#64;Test
 *         &#64;SmallTest
 *         public void testCaptureTabSwitcher() throws IOException, InterruptedException {
 *             mScreenShooter.shoot("NTP");
 *             Espresso.onView(ViewMatchers.withId(R.id.tab_switcher_button))
 *                     .perform(ViewActions.click());
 *             mScreenShooter.shoot("Tab_switcher");
 *         }
 *     }
 * }
 * </pre>
 */
public class ScreenShooter extends TestWatcher {
    private static final String SCREENSHOT_DIR =
            "org.chromium.base.test.util.Screenshooter.ScreenshotDir";
    private Instrumentation mInstrumentation;
    private UiDevice mDevice;
    private File mDir;
    private String mBaseDir;

    public ScreenShooter() {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mDevice = UiDevice.getInstance(mInstrumentation);
        mBaseDir = InstrumentationRegistry.getArguments().getString(SCREENSHOT_DIR);
    }

    @Override
    protected void starting(Description d) {
        File classDir = new File(mBaseDir, d.getClassName());
        mDir = new File(classDir, d.getMethodName());
        mDir.mkdirs();
    }

    /**
     * Take a screen shot and save it to a file.
     *
     * @param shotName The name of this particular screenshot within this test. This will be used to
     *            name the image file.
     */
    public void shoot(String shotName) {
        assertNotNull("ScreenShooter rule initialized", mDir);
        assertTrue("Screenshot " + shotName,
                mDevice.takeScreenshot(new File(mDir, imageName(shotName))));
    }

    private String imageName(String shotName) {
        int orientation =
                InstrumentationRegistry.getContext().getResources().getConfiguration().orientation;
        String orientationName =
                orientation == Configuration.ORIENTATION_LANDSCAPE ? "land" : "port";
        return String.format(
                "%s.%s.%s.png", shotName, Build.MODEL.replace(' ', '_'), orientationName);
    }
}
