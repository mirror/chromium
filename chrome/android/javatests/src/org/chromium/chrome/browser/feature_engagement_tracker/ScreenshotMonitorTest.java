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

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.io.File;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
/**
 * Tests ScreenshotMonitor.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class ScreenshotMonitorTest {
    private static final String FILENAME = "image.jpeg";
    private static final String TAG = "ScreenshotTest";

    private ScreenshotMonitor mTestScreenshotMonitor;
    private TestScreenshotMonitorDelegate mTestScreenshotMonitorDelegate;
    private TestScreenshotMonitorFileObserver mTestFileObserver;

    /**
     * This class acts as the inner FileObserver used by TestScreenshotMonitor to test that
     * ScreenshotMonitor calls the FileObserver.
     */
    static class TestScreenshotMonitorFileObserver
            extends ScreenshotMonitor.ScreenshotMonitorFileObserver {
        // The number of times watching occurs. Is is modified on the UI thread and accessed on
        // the test thread.
        public AtomicInteger mWatchingCount = new AtomicInteger();

        // Note: FileObserver's startWatching will have no effect if monitoring started already.
        @Override
        public void startWatching() {
            mWatchingCount.getAndIncrement();
        }

        // Note: FileObserver's stopWatching will have no effect if monitoring stopped already.
        @Override
        public void stopWatching() {
            mWatchingCount.getAndDecrement();
        }
    }

    static class TestScreenshotMonitorDelegate implements ScreenshotMonitorDelegate {
        // This is modified on the UI thread and accessed on the test thread.
        public AtomicInteger mScreenshotShowUiCount = new AtomicInteger();

        public void onScreenshotTaken() {
            Assert.assertTrue(ThreadUtils.runningOnUiThread());
            mScreenshotShowUiCount.getAndIncrement();
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
     * Verify that if monotoring starts, the delegate should be called. Also verifies that the
     * inner TestFileObserver monitors as expected.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagementTracker", "Screenshot"})
    public void testStartMonitoringShouldTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.mWatchingCount.get());

        startMonitoringOnUiThreadBlocking();
        Assert.assertEquals(1, mTestFileObserver.mWatchingCount.get());
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(1);

        stopMonitoringOnUiThreadBlocking();
        Assert.assertEquals(0, mTestFileObserver.mWatchingCount.get());
    }

    /**
     * Verify that if monotoring starts multiple times, the delegate should still be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagementTracker", "Screenshot"})
    public void testMultipleStartMonitoringShouldTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.mWatchingCount.get());

        startMonitoringOnUiThreadBlocking();
        startMonitoringOnUiThreadBlocking();
        Assert.assertEquals(2, mTestFileObserver.mWatchingCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(1);
    }

    /**
     * Verify that if monotoring stops multiple times, the delegate should not be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagementTracker", "Screenshot"})
    public void testMultipleStopMonitoringShouldNotTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.mWatchingCount.get());

        stopMonitoringOnUiThreadBlocking();
        stopMonitoringOnUiThreadBlocking();
        Assert.assertEquals(-2, mTestFileObserver.mWatchingCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(0);
    }

    /**
     * Verify that even if startMonitoring is called multiple times before stopMonitoring, the
     * delegate should not be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagementTracker", "Screenshot"})
    public void testMultipleStartMonitoringThenStopShouldNotTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.mWatchingCount.get());
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount.get());

        multipleStartMonitoringBeforeStopMonitoringOnUiThreadBlocking(2);
        Assert.assertEquals(1, mTestFileObserver.mWatchingCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(0);
    }

    /**
     * Verify that if stopMonitoring is called multiple times before startMonitoring, the delegate
     * should still be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagementTracker", "Screenshot"})
    public void testMultipleStopMonitoringThenStartShouldTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.mWatchingCount.get());

        multipleStopMonitoringBeforeStartMonitoringOnUiThreadBlocking(2);
        Assert.assertEquals(-1, mTestFileObserver.mWatchingCount.get());
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(1);
    }

    /**
     * Verify that if monitoring stops, the delegate should not be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagementTracker", "Screenshot"})
    public void testStopMonitoringShouldNotTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.mWatchingCount.get());

        startMonitoringOnUiThreadBlocking();
        Assert.assertEquals(1, mTestFileObserver.mWatchingCount.get());
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount.get());

        stopMonitoringOnUiThreadBlocking();
        Assert.assertEquals(0, mTestFileObserver.mWatchingCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(0);
    }

    /**
     * Verify that if monitoring is never started, the delegate should not be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagementTracker", "Screenshot"})
    public void testNoMonitoringShouldNotTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.mWatchingCount.get());
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(0);

        Assert.assertEquals(0, mTestFileObserver.mWatchingCount.get());
    }

    // This ensures that the UI thread finishes executing startMonitoring.
    private void startMonitoringOnUiThreadBlocking() {
        final Semaphore semaphore = new Semaphore(1);
        try {
            semaphore.acquire();
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }

        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                mTestScreenshotMonitor.startMonitoring();
                semaphore.release();
            }
        });
        try {
            semaphore.tryAcquire(10, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }
    }

    // This ensures that the UI thread finishes executing stopMonitoring.
    private void stopMonitoringOnUiThreadBlocking() {
        final Semaphore semaphore = new Semaphore(1);
        try {
            semaphore.acquire();
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }

        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                mTestScreenshotMonitor.stopMonitoring();
                semaphore.release();
            }
        });
        try {
            semaphore.tryAcquire(10, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }
    }

    // This ensures that the UI thread finishes executing startCallCount calls to startMonitoring
    // before calling stopMonitoring.
    private void multipleStartMonitoringBeforeStopMonitoringOnUiThreadBlocking(
            final int startCallCount) {
        final Semaphore semaphore = new Semaphore(1);
        try {
            semaphore.acquire();
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }

        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                for (int i = 0; i < startCallCount; i++) {
                    mTestScreenshotMonitor.startMonitoring();
                }
                mTestScreenshotMonitor.stopMonitoring();
                semaphore.release();
            }
        });
        try {
            semaphore.tryAcquire(10, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }
    }

    // This ensures that the UI thread finishes executing stopCallCount calls to stopMonitoring
    // before calling startMonitoring.
    private void multipleStopMonitoringBeforeStartMonitoringOnUiThreadBlocking(
            final int stopCallCount) {
        final Semaphore semaphore = new Semaphore(1);
        try {
            semaphore.acquire();
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }

        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                for (int i = 0; i < stopCallCount; i++) {
                    mTestScreenshotMonitor.stopMonitoring();
                }
                mTestScreenshotMonitor.startMonitoring();
                semaphore.release();
            }
        });
        try {
            semaphore.tryAcquire(10, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }
    }

    // This ensures that after UI thread finishes all tasks, mScreenshotShowUiCount equals
    // expectedCount.
    private void assertScreenshotShowUiCountOnUiThreadBlocking(int expectedCount) {
        final Semaphore semaphore = new Semaphore(1);
        try {
            semaphore.acquire();
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }

        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                semaphore.release();
            }
        });
        try {
            semaphore.tryAcquire(10, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }
        Assert.assertEquals(
                expectedCount, mTestScreenshotMonitorDelegate.mScreenshotShowUiCount.get());
    }
}