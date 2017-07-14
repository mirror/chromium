// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.Notification;
import android.support.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.Feature;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/**
 * Test for DownloadServiceManager
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public final class DownloadServiceManagerTest {
    private static final int FAKE_DOWNLOAD_1 = 111;
    private static final int FAKE_DOWNLOAD_2 = 222;
    private static final int FAKE_DOWNLOAD_3 = 333;

    private MockDownloadServiceManager mDownloadServiceManager;
    private Notification mNotification;

    static class MockDownloadServiceManager extends DownloadServiceManager {
        boolean mIsForegroundActive = false;

        MockDownloadServiceManager() {}

        @Override
        void startForegroundInternal(int notificationId, Notification notification) {
            mIsForegroundActive = true;
        }

        @Override
        void stopForegroundInternal() {
            mIsForegroundActive = false;
        }
    }

    @Before
    public void setUp() throws Exception {
        mDownloadServiceManager = new MockDownloadServiceManager();
        mNotification = new Notification();
    }

    @Test
    @SmallTest
    @Feature({"Download"})
    public void testStartAndStopService() {
        // Service starts and stops with addition and removal of one active download.
        mDownloadServiceManager.updateDownloadStatus(true, FAKE_DOWNLOAD_1, mNotification);
        assertTrue(mDownloadServiceManager.mIsForegroundActive);
        mDownloadServiceManager.updateDownloadStatus(false, FAKE_DOWNLOAD_1, mNotification);
        assertFalse(mDownloadServiceManager.mIsForegroundActive);

        // Service does not get affected by addition of inactive download.
        mDownloadServiceManager.updateDownloadStatus(true, FAKE_DOWNLOAD_1, mNotification);
        assertTrue(mDownloadServiceManager.mIsForegroundActive);
        mDownloadServiceManager.updateDownloadStatus(false, FAKE_DOWNLOAD_2, mNotification);
        assertTrue(mDownloadServiceManager.mIsForegroundActive);

        // Service continues as long as there is at least one active download.
        mDownloadServiceManager.updateDownloadStatus(true, FAKE_DOWNLOAD_3, mNotification);
        assertTrue(mDownloadServiceManager.mIsForegroundActive);
        mDownloadServiceManager.updateDownloadStatus(false, FAKE_DOWNLOAD_1, mNotification);
        assertTrue(mDownloadServiceManager.mIsForegroundActive);
        mDownloadServiceManager.updateDownloadStatus(false, FAKE_DOWNLOAD_3, mNotification);
        assertFalse(mDownloadServiceManager.mIsForegroundActive);
    }
}
