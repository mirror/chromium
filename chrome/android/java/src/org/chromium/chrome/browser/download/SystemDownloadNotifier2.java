// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.content.Context;

import org.chromium.components.offline_items_collection.ContentId;

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
        final int notificationId = mDownloadNotificationService.notifyDownloadSuccessful(
                DownloadInfo.Builder.fromDownloadInfo(info)
                        .setSystemDownloadId(systemDownloadId)
                        .setCanResolve(canResolve)
                        .setIsSupportedMimeType(isSupportedMimeType)
                        .build());

        if (info.getIsOpenable()) {
            DownloadManagerService.getDownloadManagerService().onSuccessNotificationShown(
                    info, canResolve, notificationId, systemDownloadId);
        }
    }

    @Override
    public void notifyDownloadFailed(DownloadInfo info) {
        mDownloadNotificationService.notifyDownloadFailed(info);
    }

    @Override
    public void notifyDownloadProgress(
            DownloadInfo info, long startTime, boolean canDownloadWhileMetered) {
        info = DownloadInfo.Builder.fromDownloadInfo(info)
                       .setStartTime(startTime)
                       .setCanDownloadWhileMetered(canDownloadWhileMetered)
                       .build();

        mDownloadNotificationService.notifyDownloadProgress(info);
    }

    @Override
    public void notifyDownloadPaused(DownloadInfo info) {
        info = DownloadInfo.Builder.fromDownloadInfo(info)
                       .setIsResumable(true)
                       .setIsAutoResumable(false)
                       .build();
        mDownloadNotificationService.notifyDownloadPaused(info, false);
    }

    @Override
    public void notifyDownloadInterrupted(DownloadInfo info, boolean isAutoResumable) {
        mDownloadNotificationService.notifyDownloadPaused(info, false);
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
