// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static org.chromium.chrome.browser.notifications.NotificationConstants.DEFAULT_NOTIFICATION_ID;

import android.app.Notification;
import android.content.Intent;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.components.offline_items_collection.ContentId;

import java.util.ArrayList;
import java.util.List;

/**
 * Mock class to DownloadNotificationService for testing purpose.
 */
public class MockDownloadNotificationService2 extends DownloadNotificationService2 {
    private final List<Integer> mNotificationIds = new ArrayList<Integer>();
    private boolean mPaused = false;
    private int mLastNotificationId = DEFAULT_NOTIFICATION_ID;

    List<String> mResumedDownloads = new ArrayList<>();

    @Override
    void updateNotification(int id, Notification notification) {
        if (!mNotificationIds.contains(id)) {
            mNotificationIds.add(id);
            mLastNotificationId = id;
        }
    }

    public boolean isPaused() {
        return mPaused;
    }

    public List<Integer> getNotificationIds() {
        return mNotificationIds;
    }

    public int getLastNotificationId() {
        return mLastNotificationId;
    }

    @Override
    public void cancelNotification(int notificationId, ContentId id) {
        super.cancelNotification(notificationId, id);
        mNotificationIds.remove(Integer.valueOf(notificationId));
    }

    @Override
    public int notifyDownloadSuccessful(final DownloadInfo downloadInfo) {
        return ThreadUtils.runOnUiThreadBlockingNoException(
                ()
                        -> MockDownloadNotificationService2.super.notifyDownloadSuccessful(
                                downloadInfo));
    }

    @Override
    public void notifyDownloadProgress(final DownloadInfo downloadInfo) {
        ThreadUtils.runOnUiThreadBlocking(
                () -> MockDownloadNotificationService2.super.notifyDownloadProgress(downloadInfo));
    }

    @Override
    void notifyDownloadPaused(final DownloadInfo downloadInfo, boolean hasUserGesture) {
        ThreadUtils.runOnUiThreadBlocking(
                ()
                        -> MockDownloadNotificationService2.super.notifyDownloadPaused(
                                downloadInfo, hasUserGesture));
    }

    @Override
    public void notifyDownloadFailed(final DownloadInfo downloadInfo) {
        ThreadUtils.runOnUiThreadBlocking(
                () -> MockDownloadNotificationService2.super.notifyDownloadFailed(downloadInfo));
    }

    @Override
    public void notifyDownloadCanceled(final ContentId id, boolean hasUserGesture) {
        ThreadUtils.runOnUiThreadBlocking(
                ()
                        -> MockDownloadNotificationService2.super.notifyDownloadCanceled(
                                id, hasUserGesture));
    }

    @Override
    void resumeDownload(Intent intent) {
        mResumedDownloads.add(IntentUtils.safeGetStringExtra(intent, EXTRA_DOWNLOAD_CONTENTID_ID));
    }
}
