// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feature_engagement_tracker;

import android.os.Environment;
import android.os.FileObserver;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.Timeout;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.io.File;

/**
 * Tests ScreenshotMonitor.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@RetryOnFailure
public class ScreenshotMonitorTest {
    private static String sFILENAME = "image.jpeg";
    private TestScreenshotMonitor mTestScreenshotMonitor;
    private TestScreenshotMonitorDelegate mTestScreenshotMonitorDelegate;

    @Rule
    public Timeout globalTimeout = Timeout.seconds(10);

    /**
     * This class acts as an interceptor for ScreenshotMonitor to test that it detects
     * screenshot as a FileObserver.
     */
    static class TestScreenshotMonitor extends ScreenshotMonitor {
        /**
         * The number of times Chrome is in the foreground. Should reset to 0 each time
         * monitoring is stopped.
         */
        public int mChromeInForegroundCount;

        public TestScreenshotMonitor(TestScreenshotMonitorDelegate delegate) {
            super(delegate);
        }

        @Override
        protected void postOnUi() {
            ThreadUtils.postOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mDelegate.onScreenshotTaken();
                    synchronized (TestScreenshotMonitor.this) {
                        TestScreenshotMonitor.this.notify();
                    }
                }
            });
        }

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

    @Before
    public void setUp() throws Exception {
        mTestScreenshotMonitorDelegate = new TestScreenshotMonitorDelegate();
        mTestScreenshotMonitorDelegate.mScreenshotShowUiCount = 0;

        mTestScreenshotMonitor = new TestScreenshotMonitor(mTestScreenshotMonitorDelegate);
        mTestScreenshotMonitor.mChromeInForegroundCount = 0;
    }

    /**
     * Verify that ScreenshotMonitor's startWatching and stopWatching are called when
     * startMonitoring and stopMonitoring are called and it notifies ScreenshotMonitorDelegate
     * when a screenshot is taken.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagementTracker", "Screenshot"})
    public void testNormalOnEventShouldTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestScreenshotMonitor.mChromeInForegroundCount);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mTestScreenshotMonitor.startMonitoring();
            }
        });
        Assert.assertEquals(1, mTestScreenshotMonitor.mChromeInForegroundCount);
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount);

        mTestScreenshotMonitor.onEvent(FileObserver.CREATE,
                Environment.getExternalStorageDirectory().getPath() + File.separator
                        + Environment.DIRECTORY_PICTURES + File.separator + "Screenshots"
                        + File.separator + sFILENAME);
        synchronized (mTestScreenshotMonitor) {
            mTestScreenshotMonitor.wait();
        }

        Assert.assertEquals(1, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount);

        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                mTestScreenshotMonitor.stopMonitoring();
                Assert.assertEquals(0, mTestScreenshotMonitor.mChromeInForegroundCount);
            }
        });
    }

    /**
     * Verify that if monitoring stopped, the delegate should not be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagementTracker", "Screenshot"})
    public void testStopMonitoringShouldNotTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestScreenshotMonitor.mChromeInForegroundCount);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mTestScreenshotMonitor.startMonitoring();
            }
        });
        Assert.assertEquals(1, mTestScreenshotMonitor.mChromeInForegroundCount);
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mTestScreenshotMonitor.stopMonitoring();
                Assert.assertEquals(0, mTestScreenshotMonitor.mChromeInForegroundCount);
            }
        });

        mTestScreenshotMonitor.onEvent(FileObserver.CREATE,
                Environment.getExternalStorageDirectory().getPath() + File.separator
                        + Environment.DIRECTORY_PICTURES + File.separator + "Screenshots"
                        + File.separator + sFILENAME);
        // Check that onScreenshotTaken is never called
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
        Assert.assertEquals(0, mTestScreenshotMonitor.mChromeInForegroundCount);
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount);

        mTestScreenshotMonitor.onEvent(FileObserver.CREATE,
                Environment.getExternalStorageDirectory().getPath() + File.separator
                        + Environment.DIRECTORY_PICTURES + File.separator + "Screenshots"
                        + File.separator + sFILENAME);
        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                Assert.assertEquals(0, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount);
            }
        });
        Assert.assertEquals(0, mTestScreenshotMonitor.mChromeInForegroundCount);
    }
}
