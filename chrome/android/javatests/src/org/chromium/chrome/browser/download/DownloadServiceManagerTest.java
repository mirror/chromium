// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.app.Notification;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.support.test.filters.SmallTest;
import android.test.ServiceTestCase;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.util.AdvancedMockContext;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.notifications.NotificationBuilderFactory;
import org.chromium.chrome.browser.notifications.channels.ChannelDefinitions;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.util.ArrayList;
import java.util.List;

/**
 * Test for DownloadServiceManager
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public final class DownloadServiceManagerTest
        extends ServiceTestCase<DownloadServiceManagerTest.MockDownloadServiceManager> {
    private static final int FAKE_DOWNLOAD_1 = 111;
    private static final int FAKE_DOWNLOAD_2 = 222;
    private static final int FAKE_DOWNLOAD_3 = 333;
    private static final int FAKE_FLAGS = 0;
    private static final int FAKE_START_ID = 0;
    private static final String FAKE_NOTIFICATION_CHANNEL = "DownloadServiceManagerTest";

    private MockDownloadServiceManager mDownloadServiceManager;
    private Notification mNotification;
    private Context mContext;
    private List<Intent> mStartServiceIntents = new ArrayList<>();

    public DownloadServiceManagerTest() {
        super(MockDownloadServiceManager.class);
    }

    /**
     * Implementation of DownloadServiceManager for testing purposes.
     * Generally mimics behavior of DownloadServiceManager, except:
     *      - Adds tracking (ie. number of times startForegroundInternal is called).
     *      - Allows delayed onStartCommand with variable mRunOnStartCommand.
     */
    public static class MockDownloadServiceManager extends DownloadServiceManager {
        boolean mIsForegroundInternalActive = false;
        int mNumStopForegroundInternalCalls = 0;
        List<Intent> mOnStartCommands = new ArrayList<>();
        boolean mRunOnStartCommand = false;

        public MockDownloadServiceManager() {}

        @Override
        void startForegroundInternal(
                Context context, int notificationId, Notification notification) {
            mIsForegroundInternalActive = true;
            super.startForegroundInternal(context, notificationId, notification);
        }

        @Override
        void stopForegroundInternal() {
            mIsForegroundInternalActive = false;
            mNumStopForegroundInternalCalls += 1;
            super.stopForegroundInternal();
        }

        @Override
        public int onStartCommand(Intent intent, int flags, int startId) {
            mOnStartCommands.add(intent);
            if (mRunOnStartCommand) {
                return super.onStartCommand(intent, flags, startId);
            }
            return START_STICKY;
        }
    }

    @Override
    public Context getSystemContext() {
        return super.getSystemContext();
    }

    @Before
    @Override
    public void setUp() throws Exception {
        mDownloadServiceManager = new MockDownloadServiceManager();
        mDownloadServiceManager.mIsJUnitTest = true;

        super.setUp();
        setContext(new AdvancedMockContext(getSystemContext()) {
            @Override
            public ComponentName startService(Intent service) {
                Intent intent = new Intent();
                intent.setComponent(new ComponentName(mContext, DownloadServiceManagerTest.class));

                DownloadServiceManagerTest.super.startService(intent);
                mStartServiceIntents.add(intent);
                mDownloadServiceManager.onStartCommand(intent, FAKE_FLAGS, FAKE_START_ID);
                return super.startService(intent);
            }
        });
        ContextUtils.initApplicationContextForTests(getContext().getApplicationContext());
        setupService();

        mNotification =
                NotificationBuilderFactory
                        .createChromeNotificationBuilder(
                                true /* preferCompat */, ChannelDefinitions.CHANNEL_ID_DOWNLOADS)
                        .setSmallIcon(org.chromium.chrome.R.drawable.ic_file_download_white_24dp)
                        .setContentTitle(FAKE_NOTIFICATION_CHANNEL)
                        .setContentText(FAKE_NOTIFICATION_CHANNEL)
                        .build();
        mContext = new AdvancedMockContext(getContext());
    }

    @Test
    @SmallTest
    @Feature({"Download"})
    public void testStartAndStopForegroundInternal() {
        // Basic test that just checks whether start/StopForegroundInternal are called.
        mDownloadServiceManager.mRunOnStartCommand = true;

        // Service starts and stops with addition and removal of one active download.
        mDownloadServiceManager.updateDownloadStatus(
                mContext, true, FAKE_DOWNLOAD_1, mNotification);
        assertTrue(mDownloadServiceManager.mIsForegroundInternalActive);
        mDownloadServiceManager.updateDownloadStatus(
                mContext, false, FAKE_DOWNLOAD_1, mNotification);
        assertFalse(mDownloadServiceManager.mIsForegroundInternalActive);

        // Service does not get affected by addition of inactive download.
        mDownloadServiceManager.updateDownloadStatus(
                mContext, true, FAKE_DOWNLOAD_1, mNotification);
        assertTrue(mDownloadServiceManager.mIsForegroundInternalActive);
        mDownloadServiceManager.updateDownloadStatus(
                mContext, false, FAKE_DOWNLOAD_2, mNotification);
        assertTrue(mDownloadServiceManager.mIsForegroundInternalActive);

        // Service continues as long as there is at least one active download.
        mDownloadServiceManager.updateDownloadStatus(
                mContext, true, FAKE_DOWNLOAD_3, mNotification);
        assertTrue(mDownloadServiceManager.mIsForegroundInternalActive);
        mDownloadServiceManager.updateDownloadStatus(
                mContext, false, FAKE_DOWNLOAD_1, mNotification);
        assertTrue(mDownloadServiceManager.mIsForegroundInternalActive);
        mDownloadServiceManager.updateDownloadStatus(
                mContext, false, FAKE_DOWNLOAD_3, mNotification);
        assertFalse(mDownloadServiceManager.mIsForegroundInternalActive);
    }

    @Test
    @SmallTest
    @Feature({"Download"})
    public void testDelayedStartStopForeground() {
        // Call start/stopForegroundInternal.
        assertFalse(mDownloadServiceManager.getIsForegroundActive());
        mDownloadServiceManager.updateDownloadStatus(
                mContext, true, FAKE_DOWNLOAD_1, mNotification);
        mDownloadServiceManager.updateDownloadStatus(
                mContext, false, FAKE_DOWNLOAD_1, mNotification);

        assertEquals(1, mDownloadServiceManager.mOnStartCommands.size());
        assertEquals(1, mDownloadServiceManager.mNumStopForegroundInternalCalls);

        // Service actually starts.
        mDownloadServiceManager.mRunOnStartCommand = true;
        mDownloadServiceManager.onStartCommand(
                mDownloadServiceManager.mOnStartCommands.get(0), FAKE_FLAGS, FAKE_START_ID);

        // Make sure the service gets shut down immediately.
        assertEquals(2, mDownloadServiceManager.mNumStopForegroundInternalCalls);
        assertFalse(mDownloadServiceManager.getIsForegroundActive());
    }

    @Test
    @SmallTest
    @Feature({"Download"})
    public void testDelayedStartStopStartForeground() {
        // Call start/stop/startForegroundInternal.
        mDownloadServiceManager.updateDownloadStatus(
                mContext, true, FAKE_DOWNLOAD_1, mNotification);
        mDownloadServiceManager.updateDownloadStatus(
                mContext, false, FAKE_DOWNLOAD_1, mNotification);
        mDownloadServiceManager.updateDownloadStatus(
                mContext, true, FAKE_DOWNLOAD_2, mNotification);

        // Make sure that startService only gets called once.
        assertEquals(1, mStartServiceIntents.size());
        assertEquals(1, mDownloadServiceManager.mOnStartCommands.size());

        // Service actually starts.
        mDownloadServiceManager.mRunOnStartCommand = true;
        mDownloadServiceManager.onStartCommand(
                mDownloadServiceManager.mOnStartCommands.get(0), FAKE_FLAGS, FAKE_START_ID);

        // Make sure service continues and is pinned to second download.
        assertTrue(mDownloadServiceManager.getIsForegroundActive());
        assertEquals(FAKE_DOWNLOAD_2, mDownloadServiceManager.getPinnedNotificationId());

        // Make sure service is able to be shut down.
        mDownloadServiceManager.updateDownloadStatus(
                mContext, false, FAKE_DOWNLOAD_2, mNotification);
        assertEquals(2, mDownloadServiceManager.mNumStopForegroundInternalCalls);
        assertFalse(mDownloadServiceManager.getIsForegroundActive());
    }
}
