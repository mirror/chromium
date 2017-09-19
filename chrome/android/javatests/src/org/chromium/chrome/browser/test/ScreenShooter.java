// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.test;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.app.Instrumentation;
import android.content.res.Configuration;
import android.graphics.Point;
import android.os.Build;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.UiDevice;

import org.json.JSONException;
import org.json.JSONObject;
import org.junit.rules.TestWatcher;
import org.junit.runner.Description;

import org.chromium.chrome.browser.ChromeVersionInfo;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.text.DateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/**
 * Rule for taking screen shots within tests. Screenshots are saved as
 * UiCapture/<test class directory>/<test directory>/<shot name>.png.
 * <p>
 * <test class directory> and <test directory> can both the set by the @ScreenShooter.Directory
 * annotation. <test class directory> defaults to nothing (i.e. no directory created at this
 * level), and <test directory> defaults to the name of the individual test.
 * <p>
 * A simple example:
 * <p>
 * <pre>
 * {
 * @RunWith(ChromeJUnit4ClassRunner.class)
 * @CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
 * @Restriction(RESTRICTION_TYPE_PHONE) // Tab switcher button only exists on phones.
 * @ScreenShooter.Directory("Example")
 * public class ExampleUiCaptureTest {
 *     @Rule
 *     public ChromeActivityTestRule<ChromeTabbedActivity> mActivityTestRule =
 *             new ChromeActivityTestRule<>(ChromeTabbedActivity.class);
 *
 *     @Rule
 *     public ScreenShooter mScreenShooter = new ScreenShooter();
 *
 *     @Before
 *     public void setUp() throws InterruptedException {
 *         mActivityTestRule.startMainActivityFromLauncher();
 *     }
 *
 *     // Capture the New Tab Page and the tab switcher.
 *     @Test
 *     @SmallTest
 *     @Feature({"UiCatalogue"})
 *     @ScreenShooter.Directory("TabSwitcher")
 *     public void testCaptureTabSwitcher() throws IOException, InterruptedException {
 *         mScreenShooter.shoot("NTP");
 *         Espresso.onView(ViewMatchers.withId(R.id.tab_switcher_button)).
 *                      perform(ViewActions.click());
 *         mScreenShooter.shoot("Tab switcher");
 *     }
 * }
 * }
 * </pre>
 */
public class ScreenShooter extends TestWatcher {
    private static final String SCREENSHOT_DIR =
            "org.chromium.base.test.util.Screenshooter.ScreenshotDir";
    private static final String IMAGE_SUFFIX = ".png";
    private static final String JSON_SUFFIX = ".json";
    private final Instrumentation mInstrumentation;
    private final UiDevice mDevice;
    private final String mBaseDir;
    private File mDir;
    private String mTestClassName;
    private String mTestMethodName;

    @Retention(RetentionPolicy.RUNTIME)
    @Target({ElementType.TYPE, ElementType.METHOD})
    public @interface Directory {
        String value();
    }

    public ScreenShooter() {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mDevice = UiDevice.getInstance(mInstrumentation);
        mBaseDir = InstrumentationRegistry.getArguments().getString(SCREENSHOT_DIR);
    }

    @Override
    protected void starting(Description d) {
        mDir = new File(mBaseDir);
        mTestClassName = d.getClassName();
        mTestMethodName = d.getMethodName();
        Class<?> testClass = d.getTestClass();
        Directory classDirectoryAnnotation = testClass.getAnnotation(Directory.class);
        String classDirName =
                classDirectoryAnnotation == null ? "" : classDirectoryAnnotation.value();
        if (!classDirName.isEmpty()) mDir = new File(mBaseDir, classDirName);
        Directory methodDirectoryAnnotation = d.getAnnotation(Directory.class);
        String testMethodDir = methodDirectoryAnnotation == null
                ? d.getMethodName()
                : methodDirectoryAnnotation.value();
        if (!testMethodDir.isEmpty()) mDir = new File(mDir, testMethodDir);
        if (!mDir.exists()) assertTrue("Create screenshot directory", mDir.mkdirs());
    }

    /**
     * Take a screenshot and save it to a file, with tags and metadata in a JSON file
     *
     * @param shotName The name of this particular screenshot within this test.
     */
    public void shoot(String shotName) {
        assertNotNull("ScreenShooter rule initialized", mDir);

        Map<String, String> tags = new HashMap<>();
        shoot(shotName, tags);
    }

    /**
     * Take a screenshot and save it to a file, with tags and metadata in a JSON file
     *
     * @param shotName The name of this particular screenshot within this test.
     * @param tags User defined tags, must not clash with default tags.
     */
    public void shoot(String shotName, Map<String, String> tags) {
        tags.put("Test Class", mTestClassName);
        tags.put("Test Method", mTestMethodName);
        tags.put("Screenshot Name", shotName);
        tags.put("Device Model", Build.MANUFACTURER + " " + Build.MODEL);
        Point displaySize = mDevice.getDisplaySizeDp();
        tags.put("Display Size",
                String.format(Locale.US, "%d X %d", Math.min(displaySize.x, displaySize.y),
                        Math.max(displaySize.x, displaySize.y)));
        int orientation =
                InstrumentationRegistry.getContext().getResources().getConfiguration().orientation;
        tags.put("Orientation",
                orientation == Configuration.ORIENTATION_LANDSCAPE ? "landscape" : "portrait");
        tags.put("Android Version", Build.VERSION.RELEASE);
        tags.put("Chrome Version", Integer.toString(ChromeVersionInfo.getProductMajorVersion()));
        String channelName = "Unknown";
        if (ChromeVersionInfo.isLocalBuild()) {
            channelName = "Local Build";
        } else if (ChromeVersionInfo.isCanaryBuild()) {
            channelName = "Canary";
        } else if (ChromeVersionInfo.isBetaBuild()) {
            channelName = "Beta";
        } else if (ChromeVersionInfo.isDevBuild()) {
            channelName = "Dev";
        } else if (ChromeVersionInfo.isStableBuild()) {
            channelName = "Stable";
        }
        if (ChromeVersionInfo.isOfficialBuild()) {
            channelName = channelName + " Official";
        }
        tags.put("Chrome Channel", channelName);
        tags.put("Locale", Locale.getDefault().toString());

        Map<String, String> metadata = new HashMap<>();
        DateFormat formatter =
                DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT, Locale.US);
        metadata.put("Capture time (UTC)", formatter.format(new Date()));
        metadata.put("Chrome full product version", ChromeVersionInfo.getProductVersion());
        metadata.put("Android build fingerprint", Build.FINGERPRINT);

        try {
            File shotFile = File.createTempFile(shotName, IMAGE_SUFFIX, mDir);
            assertTrue("Screenshot " + shotName, mDevice.takeScreenshot(shotFile));
            writeImageDesciption(shotFile, tags, metadata);
        } catch (IOException e) {
            fail("Cannot create shot file");
        }
    }

    private void writeImageDesciption(
            File shotFile, Map<String, String> tags, Map<String, String> metadata) {
        JSONObject imageDescription = new JSONObject();
        String shotFileName = shotFile.getName();
        try {
            imageDescription.put("location", shotFileName);
            imageDescription.put("tags", new JSONObject(tags));
            imageDescription.put("metadata", new JSONObject(metadata));
        } catch (JSONException e) {
            fail("JSON error");
        }
        String jsonFileName =
                shotFileName.substring(0, shotFileName.length() - IMAGE_SUFFIX.length())
                + JSON_SUFFIX;
        try (FileWriter fileWriter = new FileWriter(new File(mDir, jsonFileName));) {
            fileWriter.write(imageDescription.toString());
        } catch (IOException e) {
            fail("Cannot create JSON file");
        }
    }
}
