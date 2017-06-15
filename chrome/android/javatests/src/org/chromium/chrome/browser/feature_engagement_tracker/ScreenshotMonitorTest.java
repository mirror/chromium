// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feature_engagement_tracker;

import android.os.FileObserver;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.io.File;

/**
 * Tests ScreenshotMonitor.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class ScreenshotMonitorTest {
    private static final String FILENAME = "image.jpeg";

    private ScreenshotMonitor mTestScreenshotMonitor;
    private TestScreenshotMonitorDelegate mTestScreenshotMonitorDelegate;
    private TestScreenshotMonitorFileObserver mTestFileObserver;

    /**
     * This class acts as the inner FileObserver used by TestScreenshotMonitor to test that
     * ScreenshotMonitor calls the FileObserver.
     */
    static class TestScreenshotMonitorFileObserver
            extends ScreenshotMonitor.ScreenshotMonitorFileObserver {
        /**
         * The number of times Chrome is in the foreground. Should reset to 0 each time
         * monitoring is stopped.
         */
        public int mChromeInForegroundCount;

        @Override
        public void startWatching() {
            ++mChromeInForegroundCount;
        }

        @Override
        public void stopWatching() {
            --mChromeInForegroundCount;
        }
    }

    static class TestScreenshotMonitorDelegate implements ScreenshotMonitorDelegate {
        public int mScreenshotShowUiCount;

        public void onScreenshotTaken() {
            ++mScreenshotShowUiCount;
        }
    }

    static String getTestFilePath() {
        return ScreenshotMonitor.ScreenshotMonitorFileObserver.getDirPath() + File.separator
                + FILENAME;
    }

    @Before
    public void setUp() throws Exception {
        mTestScreenshotMonitorDelegate = new TestScreenshotMonitorDelegate();

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mTestScreenshotMonitor = new ScreenshotMonitor(
                        mTestScreenshotMonitorDelegate, new TestScreenshotMonitorFileObserver());
            }
        });
        Assert.assertTrue(
                mTestScreenshotMonitor.mFileObserver instanceof TestScreenshotMonitorFileObserver);
        mTestFileObserver =
                (TestScreenshotMonitorFileObserver) mTestScreenshotMonitor.mFileObserver;
    }

    /**
     * Verify that ScreenshotMonitor's FileObserver monitors accordingly and notifies
     * ScreenshotMonitorDelegate when onEvent is called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagementTracker", "Screenshot"})
    public void testNormalOnEventShouldTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.mChromeInForegroundCount);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mTestScreenshotMonitor.startMonitoring();
            }
        });
        Assert.assertEquals(1, mTestFileObserver.mChromeInForegroundCount);
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount);

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                Assert.assertEquals(1, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount);
                mTestScreenshotMonitor.stopMonitoring();
            }
        });
        Assert.assertEquals(0, mTestFileObserver.mChromeInForegroundCount);
    }

    /**
     * Verify that if monitoring stops, the delegate should not be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagementTracker", "Screenshot"})
    public void testStopMonitoringShouldNotTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.mChromeInForegroundCount);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mTestScreenshotMonitor.startMonitoring();
            }
        });
        Assert.assertEquals(1, mTestFileObserver.mChromeInForegroundCount);
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mTestScreenshotMonitor.stopMonitoring();
            }
        });
        Assert.assertEquals(0, mTestFileObserver.mChromeInForegroundCount);

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());

        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                Assert.assertEquals(0, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount);
            }
        });
    }

    /**
     * Verify that if monitoring is never started, the delegate should not be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagementTracker", "Screenshot"})
    public void testNoMonitoringShouldNotTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.mChromeInForegroundCount);
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount);

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                Assert.assertEquals(0, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount);
            }
        });
        Assert.assertEquals(0, mTestFileObserver.mChromeInForegroundCount);
    }
}
