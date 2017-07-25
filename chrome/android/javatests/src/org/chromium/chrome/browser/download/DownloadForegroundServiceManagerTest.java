// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;

import static org.chromium.chrome.browser.download.DownloadForegroundServiceManager.DownloadStatus;
import static org.chromium.chrome.browser.notifications.NotificationConstants.DEFAULT_NOTIFICATION_ID;

import android.app.Notification;
import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.AdvancedMockContext;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.notifications.NotificationBuilderFactory;
import org.chromium.chrome.browser.notifications.channels.ChannelDefinitions;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/**
 * Test for DownloadForegroundServiceManager.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public final class DownloadForegroundServiceManagerTest {
    private static final int FAKE_DOWNLOAD_1 = 111;
    private static final int FAKE_DOWNLOAD_2 = 222;
    private static final int FAKE_DOWNLOAD_3 = 333;
    private static final String FAKE_NOTIFICATION_CHANNEL = "DownloadForegroundServiceManagerTest";

    private MockDownloadForegroundServiceManager mDownloadServiceManager;
    private Notification mNotification;
    private Context mContext;

    /**
     * Implementation of DownloadServiceManager for testing purposes.
     * Generally mimics behavior of DownloadServiceManager, except:
     *      - Adds tracking (ie. number of times startForegroundInternal is called).
     *      - Allows delayed onStartCommand with variable mRunOnStartCommand.
     */
    public static class MockDownloadForegroundServiceManager
            extends DownloadForegroundServiceManager {
        private boolean mIsServiceBound = false;
        private int mUpdatedNotificationId = DEFAULT_NOTIFICATION_ID;
        private int mNumStartForegroundCalls = 0;
        private int mNumStopForegroundCalls = 0;
        private int mNumBindServiceCalls = 0;

        public MockDownloadForegroundServiceManager(Context context) {
            super(context);
        }

        @Override
        void bindService(Context context, int notificationId, Notification notification) {
            mNumBindServiceCalls += 1;
            mIsServiceBound = true;
        }

        @Override
        void unbindService(boolean isComplete) {
            mIsServiceBound = false;
        }

        @Override
        void startDownloadForegroundService(
                Context context, int notificationId, Notification notification) {
            mNumStartForegroundCalls += 1;
            super.startDownloadForegroundService(context, notificationId, notification);
        }

        @Override
        void stopDownloadForegroundService(boolean isComplete) {
            mNumStopForegroundCalls += 1;
            super.stopDownloadForegroundService(isComplete);
        }

        @Override
        void updatePinnedNotificationIfNeeded(int notificationId, Notification notification) {
            mUpdatedNotificationId = notificationId;
        }
    }

    /**
     * Implementation of DownloadForegroundService for testing.
     */
    public static class MockDownloadForegroundService extends DownloadForegroundService {}

    private void onServiceConnected() {
        mDownloadServiceManager.setBoundService(new MockDownloadForegroundService());
        mDownloadServiceManager.handlePendingNotifications();
    }

    @Before
    public void setUp() throws Exception {
        mContext = new AdvancedMockContext(
                InstrumentationRegistry.getInstrumentation().getTargetContext());
        mDownloadServiceManager = new MockDownloadForegroundServiceManager(mContext);

        mNotification =
                NotificationBuilderFactory
                        .createChromeNotificationBuilder(
                                true /* preferCompat */, ChannelDefinitions.CHANNEL_ID_DOWNLOADS)
                        .setSmallIcon(org.chromium.chrome.R.drawable.ic_file_download_white_24dp)
                        .setContentTitle(FAKE_NOTIFICATION_CHANNEL)
                        .setContentText(FAKE_NOTIFICATION_CHANNEL)
                        .build();
    }

    @Test
    @SmallTest
    @Feature({"Download"})
    public void testBasicStartAndStop() {
        // Service starts and stops with addition and removal of one active download.
        mDownloadServiceManager.updateDownloadStatus(
                mContext, DownloadStatus.START, FAKE_DOWNLOAD_1, mNotification);
        assertEquals(1, mDownloadServiceManager.mNumStartForegroundCalls);
        assertTrue(mDownloadServiceManager.mIsServiceBound);
        onServiceConnected();

        mDownloadServiceManager.updateDownloadStatus(
                mContext, DownloadStatus.COMPLETE, FAKE_DOWNLOAD_1, mNotification);
        assertEquals(1, mDownloadServiceManager.mNumStopForegroundCalls);
        assertFalse(mDownloadServiceManager.mIsServiceBound);

        // Service does not get affected by addition of inactive download.
        mDownloadServiceManager.updateDownloadStatus(
                mContext, DownloadStatus.START, FAKE_DOWNLOAD_1, mNotification);
        assertEquals(2, mDownloadServiceManager.mNumStartForegroundCalls);
        assertTrue(mDownloadServiceManager.mIsServiceBound);
        onServiceConnected();

        mDownloadServiceManager.updateDownloadStatus(
                mContext, DownloadStatus.PAUSE, FAKE_DOWNLOAD_2, mNotification);
        assertTrue(mDownloadServiceManager.mIsServiceBound);

        // Service continues as long as there is at least one active download.
        mDownloadServiceManager.updateDownloadStatus(
                mContext, DownloadStatus.START, FAKE_DOWNLOAD_3, mNotification);
        assertTrue(mDownloadServiceManager.mIsServiceBound);
        mDownloadServiceManager.updateDownloadStatus(
                mContext, DownloadStatus.PAUSE, FAKE_DOWNLOAD_1, mNotification);
        assertTrue(mDownloadServiceManager.mIsServiceBound);
        mDownloadServiceManager.updateDownloadStatus(
                mContext, DownloadStatus.COMPLETE, FAKE_DOWNLOAD_3, mNotification);
        assertFalse(mDownloadServiceManager.mIsServiceBound);
    }

    @Test
    @SmallTest
    @Feature({"Download"})
    public void testDelayedStartStop() {
        // Calls to start and stop service.
        assertFalse(mDownloadServiceManager.mIsServiceBound);
        mDownloadServiceManager.updateDownloadStatus(
                mContext, DownloadStatus.START, FAKE_DOWNLOAD_1, mNotification);
        mDownloadServiceManager.updateDownloadStatus(
                mContext, DownloadStatus.COMPLETE, FAKE_DOWNLOAD_1, mNotification);

        assertTrue(mDownloadServiceManager.mIsServiceBound);
        assertEquals(1, mDownloadServiceManager.mNumStartForegroundCalls);
        assertEquals(1, mDownloadServiceManager.mNumStopForegroundCalls);

        // Service actually starts, should be shut down immediately.
        onServiceConnected();

        assertEquals(2, mDownloadServiceManager.mNumStopForegroundCalls);
        assertFalse(mDownloadServiceManager.mIsServiceBound);
    }

    @Test
    @SmallTest
    @Feature({"Download"})
    public void testDelayedStartStopStart() {
        // Calls to start and stop and start service.
        assertFalse(mDownloadServiceManager.mIsServiceBound);
        mDownloadServiceManager.updateDownloadStatus(
                mContext, DownloadStatus.START, FAKE_DOWNLOAD_1, mNotification);
        mDownloadServiceManager.updateDownloadStatus(
                mContext, DownloadStatus.COMPLETE, FAKE_DOWNLOAD_1, mNotification);
        mDownloadServiceManager.updateDownloadStatus(
                mContext, DownloadStatus.START, FAKE_DOWNLOAD_2, mNotification);

        // Make sure start service only gets called once.
        assertTrue(mDownloadServiceManager.mIsServiceBound);
        assertEquals(2, mDownloadServiceManager.mNumStartForegroundCalls);
        assertEquals(1, mDownloadServiceManager.mNumStopForegroundCalls);
        assertEquals(1, mDownloadServiceManager.mNumBindServiceCalls);

        // Service actually starts, continues and is pinned to second download.
        onServiceConnected();
        assertTrue(mDownloadServiceManager.mIsServiceBound);
        assertEquals(FAKE_DOWNLOAD_2, mDownloadServiceManager.mUpdatedNotificationId);

        // Make sure service is able to be shut down.
        mDownloadServiceManager.updateDownloadStatus(
                mContext, DownloadStatus.COMPLETE, FAKE_DOWNLOAD_2, mNotification);
        assertEquals(2, mDownloadServiceManager.mNumStopForegroundCalls);
        assertFalse(mDownloadServiceManager.mIsServiceBound);
    }
}
