// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.content.Context;

import org.chromium.components.offline_items_collection.ContentId;
import org.chromium.components.offline_items_collection.OfflineItem;

/**
 * DownloadNotifier implementation that creates and updates download notifications.
 * This class creates the {@link DownloadNotificationService2} when needed, and binds
 * to the latter to issue calls to show and update notifications.
 */
public class SystemDownloadNotifier2 implements DownloadNotifier {
    private final Context mApplicationContext;
    private final DownloadNotificationService2 mDownloadNotificationService;

    /**
     * Constructor.
     * @param context Application context.
     */
    public SystemDownloadNotifier2(Context context) {
        mApplicationContext = context.getApplicationContext();
        mDownloadNotificationService = DownloadNotificationService2.getInstance();
    }

    @Override
    public void notifyDownloadCanceled(ContentId id) {
        mDownloadNotificationService.notifyDownloadCanceled(id, false);
    }

    @Override
    public void notifyDownloadSuccessful(DownloadInfo info, long systemDownloadId,
            boolean canResolve, boolean isSupportedMimeType) {
        OfflineItem offlineItem = DownloadInfo.offlineItem(info);
        offlineItem.systemDownloadId = systemDownloadId;
        offlineItem.canResolve = canResolve;
        offlineItem.isSupportedMimeType = isSupportedMimeType;

        final int notificationId =
                mDownloadNotificationService.notifyDownloadSuccessful(offlineItem);

        if (info.getIsOpenable()) {
            DownloadManagerService.getDownloadManagerService().onSuccessNotificationShown(
                    info, canResolve, notificationId, systemDownloadId);
        }
    }

    @Override
    public void notifyDownloadFailed(DownloadInfo info) {
        mDownloadNotificationService.notifyDownloadFailed(DownloadInfo.offlineItem(info));
    }

    @Override
    public void notifyDownloadProgress(
            DownloadInfo info, long startTime, boolean canDownloadWhileMetered) {
        OfflineItem offlineItem = DownloadInfo.offlineItem(info);
        offlineItem.creationTimeMs = startTime;
        offlineItem.allowMetered = canDownloadWhileMetered;

        mDownloadNotificationService.notifyDownloadProgress(offlineItem);
    }

    @Override
    public void notifyDownloadPaused(DownloadInfo info) {
        OfflineItem offlineItem = DownloadInfo.offlineItem(info);
        offlineItem.isResumable = true;
        offlineItem.isAutoResumable = false;

        mDownloadNotificationService.notifyDownloadPaused(offlineItem, false);
    }

    @Override
    public void notifyDownloadInterrupted(DownloadInfo info, boolean isAutoResumable) {
        OfflineItem offlineItem = DownloadInfo.offlineItem(info);
        mDownloadNotificationService.notifyDownloadPaused(offlineItem, false);
    }

    @Override
    public void removeDownloadNotification(int notificationId, DownloadInfo info) {
        mDownloadNotificationService.cancelNotification(notificationId, info.getContentId());
    }

    @Override
    public void resumePendingDownloads() {
        if (!DownloadNotificationService2.isTrackingResumableDownloads(mApplicationContext)) return;
        mDownloadNotificationService.resumeAllPendingDownloads();
    }
}
