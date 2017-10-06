// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static android.app.DownloadManager.ACTION_NOTIFICATION_CLICKED;
import static android.app.DownloadManager.EXTRA_NOTIFICATION_CLICK_DOWNLOAD_IDS;

import static org.chromium.chrome.browser.download.DownloadNotificationService2.ACTION_DOWNLOAD_CANCEL;
import static org.chromium.chrome.browser.download.DownloadNotificationService2.ACTION_DOWNLOAD_OPEN;
import static org.chromium.chrome.browser.download.DownloadNotificationService2.ACTION_DOWNLOAD_PAUSE;
import static org.chromium.chrome.browser.download.DownloadNotificationService2.ACTION_DOWNLOAD_RESUME;
import static org.chromium.chrome.browser.download.DownloadNotificationService2.EXTRA_DOWNLOAD_CONTENTID_ID;
import static org.chromium.chrome.browser.download.DownloadNotificationService2.EXTRA_DOWNLOAD_CONTENTID_NAMESPACE;
import static org.chromium.chrome.browser.download.DownloadNotificationService2.EXTRA_DOWNLOAD_FILE_PATH;
import static org.chromium.chrome.browser.download.DownloadNotificationService2.EXTRA_IS_OFF_THE_RECORD;
import static org.chromium.chrome.browser.download.DownloadNotificationService2.EXTRA_IS_SUPPORTED_MIME_TYPE;
import static org.chromium.chrome.browser.download.DownloadNotificationService2.EXTRA_NOTIFICATION_BUNDLE_ICON_ID;
import static org.chromium.chrome.browser.notifications.NotificationConstants.INVALID_NOTIFICATION_ID;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;

import com.google.ipc.invalidation.util.Preconditions;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.notifications.ChromeNotificationBuilder;
import org.chromium.chrome.browser.notifications.NotificationBuilderFactory;
import org.chromium.chrome.browser.notifications.NotificationConstants;
import org.chromium.chrome.browser.notifications.channels.ChannelDefinitions;
import org.chromium.components.offline_items_collection.ContentId;
import org.chromium.components.offline_items_collection.LegacyHelpers;
import org.chromium.components.offline_items_collection.OfflineItem;

/**
 * Creates and updates notifications related to downloads.
 */
public final class DownloadNotificationFactory {
    // Limit file name to 25 characters. TODO(qinmin): use different limit for different devices?
    private static final int MAX_FILE_NAME_LENGTH = 25;

    // TODO(jming): Eventually move this to DownloadNotificationStore.
    enum DownloadStatus {
        IN_PROGRESS,
        PAUSED,
        SUCCESSFUL,
        FAILED,
        DELETED,
        SUMMARY // TODO(jming): Remove when summary notification is no longer in-use.
    }

    /**
     * Builds a downloads notification based on the status of the download and its information.
     * @param context of the download.
     * @param downloadStatus (in progress, paused, successful, failed, deleted, or summary).
     * @param offlineItem information about the download (ie. contentId, fileName, icon, etc).
     * @return Notification that is built based on these parameters.
     */
    static Notification buildNotification(
            Context context, DownloadStatus downloadStatus, OfflineItem offlineItem) {
        ChromeNotificationBuilder builder =
                NotificationBuilderFactory
                        .createChromeNotificationBuilder(
                                true /* preferCompat */, ChannelDefinitions.CHANNEL_ID_DOWNLOADS)
                        .setLocalOnly(true)
                        .setGroup(NotificationConstants.GROUP_DOWNLOADS)
                        .setAutoCancel(true);

        String contentText;
        int iconId;

        switch (downloadStatus) {
            case IN_PROGRESS:
                Preconditions.checkNotNull(offlineItem.progress);
                Preconditions.checkNotNull(offlineItem.id);
                Preconditions.checkArgument(offlineItem.notificationId != INVALID_NOTIFICATION_ID);

                boolean indeterminate =
                        offlineItem.progress.isIndeterminate() || offlineItem.isPending;
                if (offlineItem.isPending) {
                    contentText = context.getResources().getString(
                            R.string.download_notification_pending);
                } else if (indeterminate || offlineItem.timeRemainingMs < 0) {
                    // TODO(dimich): Enable the byte count back in M59. See bug 704049 for more info
                    // and details of what was temporarily reverted (for M58).
                    contentText = context.getResources().getString(R.string.download_started);
                } else {
                    contentText = DownloadUtils.getTimeOrFilesLeftString(
                            context, offlineItem.progress, offlineItem.timeRemainingMs);
                }
                iconId = offlineItem.isPending ? R.drawable.ic_download_pending
                                               : android.R.drawable.stat_sys_download;

                Intent pauseIntent = buildActionIntent(
                        context, ACTION_DOWNLOAD_PAUSE, offlineItem.id, offlineItem.isOffTheRecord);
                Intent cancelIntent = buildActionIntent(context, ACTION_DOWNLOAD_CANCEL,
                        offlineItem.id, offlineItem.isOffTheRecord);

                builder.setOngoing(true)
                        .setPriority(Notification.PRIORITY_HIGH)
                        .setAutoCancel(false)
                        .setLargeIcon(offlineItem.icon)
                        .addAction(R.drawable.ic_pause_white_24dp,
                                context.getResources().getString(
                                        R.string.download_notification_pause_button),
                                buildPendingIntent(
                                        context, pauseIntent, offlineItem.notificationId))
                        .addAction(R.drawable.btn_close_white,
                                context.getResources().getString(
                                        R.string.download_notification_cancel_button),
                                buildPendingIntent(
                                        context, cancelIntent, offlineItem.notificationId));

                if (!offlineItem.isPending) {
                    builder.setProgress(100,
                            indeterminate ? -1 : offlineItem.progress.getPercentage(),
                            indeterminate);
                }

                if (!indeterminate && !LegacyHelpers.isLegacyOfflinePage(offlineItem.id)) {
                    String percentText =
                            DownloadUtils.getPercentageString(offlineItem.progress.getPercentage());
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                        builder.setSubText(percentText);
                    } else {
                        builder.setContentInfo(percentText);
                    }
                }

                if (offlineItem.creationTimeMs > 0) {
                    builder.setWhen(offlineItem.creationTimeMs);
                }

                break;
            case PAUSED:
                Preconditions.checkNotNull(offlineItem.id);
                Preconditions.checkArgument(offlineItem.notificationId != INVALID_NOTIFICATION_ID);

                contentText =
                        context.getResources().getString(R.string.download_notification_paused);
                iconId = R.drawable.ic_download_pause;

                Intent resumeIntent = buildActionIntent(context, ACTION_DOWNLOAD_RESUME,
                        offlineItem.id, offlineItem.isOffTheRecord);
                cancelIntent = buildActionIntent(context, ACTION_DOWNLOAD_CANCEL, offlineItem.id,
                        offlineItem.isOffTheRecord);
                PendingIntent deleteIntent =
                        buildPendingIntent(context, cancelIntent, offlineItem.notificationId);

                builder.setAutoCancel(false)
                        .setLargeIcon(offlineItem.icon)
                        .addAction(R.drawable.ic_file_download_white_24dp,
                                context.getResources().getString(
                                        R.string.download_notification_resume_button),
                                buildPendingIntent(
                                        context, resumeIntent, offlineItem.notificationId))
                        .addAction(R.drawable.btn_close_white,
                                context.getResources().getString(
                                        R.string.download_notification_cancel_button),
                                buildPendingIntent(
                                        context, cancelIntent, offlineItem.notificationId))
                        .setDeleteIntent(deleteIntent);

                break;

            case SUCCESSFUL:
                Preconditions.checkArgument(offlineItem.notificationId != INVALID_NOTIFICATION_ID);

                contentText =
                        context.getResources().getString(R.string.download_notification_completed);
                iconId = R.drawable.offline_pin;

                if (offlineItem.isOpenable) {
                    Intent intent;
                    if (LegacyHelpers.isLegacyDownload(offlineItem.id)) {
                        Preconditions.checkNotNull(offlineItem.id);
                        Preconditions.checkArgument(offlineItem.systemDownloadId != -1);

                        intent = new Intent(ACTION_NOTIFICATION_CLICKED);
                        long[] idArray = {offlineItem.systemDownloadId};
                        intent.putExtra(EXTRA_NOTIFICATION_CLICK_DOWNLOAD_IDS, idArray);
                        intent.putExtra(EXTRA_DOWNLOAD_FILE_PATH, offlineItem.filePath);
                        intent.putExtra(
                                EXTRA_IS_SUPPORTED_MIME_TYPE, offlineItem.isSupportedMimeType);
                        intent.putExtra(EXTRA_IS_OFF_THE_RECORD, offlineItem.isOffTheRecord);
                        intent.putExtra(EXTRA_DOWNLOAD_CONTENTID_ID, offlineItem.id.id);
                        intent.putExtra(
                                EXTRA_DOWNLOAD_CONTENTID_NAMESPACE, offlineItem.id.namespace);
                        intent.putExtra(NotificationConstants.EXTRA_NOTIFICATION_ID,
                                offlineItem.notificationId);
                        DownloadUtils.setOriginalUrlAndReferralExtraToIntent(
                                intent, offlineItem.originalUrl, offlineItem.referrer);
                    } else {
                        intent = buildActionIntent(
                                context, ACTION_DOWNLOAD_OPEN, offlineItem.id, false);
                    }

                    ComponentName component = new ComponentName(
                            context.getPackageName(), DownloadBroadcastManager.class.getName());
                    intent.setComponent(component);
                    builder.setContentIntent(PendingIntent.getService(context,
                            offlineItem.notificationId, intent, PendingIntent.FLAG_UPDATE_CURRENT));
                }
                break;

            case FAILED:
                iconId = android.R.drawable.stat_sys_download_done;
                contentText =
                        context.getResources().getString(R.string.download_notification_failed);
                break;

            default:
                iconId = -1;
                contentText = "";
                break;
        }

        Bundle extras = new Bundle();
        extras.putInt(EXTRA_NOTIFICATION_BUNDLE_ICON_ID, iconId);

        builder.setContentText(contentText).setSmallIcon(iconId).addExtras(extras);

        if (offlineItem.fileName != null) {
            builder.setContentTitle(DownloadUtils.getAbbreviatedFileName(
                    offlineItem.fileName, MAX_FILE_NAME_LENGTH));
        }
        if (offlineItem.icon != null) builder.setLargeIcon(offlineItem.icon);
        if (!offlineItem.isTransient && offlineItem.notificationId != INVALID_NOTIFICATION_ID
                && downloadStatus != DownloadStatus.SUCCESSFUL
                && downloadStatus != DownloadStatus.FAILED) {
            Intent downloadHomeIntent = buildActionIntent(
                    context, ACTION_NOTIFICATION_CLICKED, null, offlineItem.isOffTheRecord);
            builder.setContentIntent(PendingIntent.getService(context, offlineItem.notificationId,
                    downloadHomeIntent, PendingIntent.FLAG_UPDATE_CURRENT));
        }

        return builder.build();
    }

    /**
     * Helper method to build a PendingIntent from the provided intent.
     * @param intent Intent to broadcast.
     * @param notificationId ID of the notification.
     */
    private static PendingIntent buildPendingIntent(
            Context context, Intent intent, int notificationId) {
        return PendingIntent.getService(
                context, notificationId, intent, PendingIntent.FLAG_UPDATE_CURRENT);
    }

    /**
     * Helper method to build an download action Intent from the provided information.
     * @param context {@link Context} to pull resources from.
     * @param action Download action to perform.
     * @param id The {@link ContentId} of the download.
     * @param isOffTheRecord Whether the download is incognito.
     */
    static Intent buildActionIntent(
            Context context, String action, ContentId id, boolean isOffTheRecord) {
        ComponentName component = new ComponentName(
                context.getPackageName(), DownloadBroadcastManager.class.getName());
        Intent intent = new Intent(action);
        intent.setComponent(component);
        intent.putExtra(EXTRA_DOWNLOAD_CONTENTID_ID, id != null ? id.id : "");
        intent.putExtra(EXTRA_DOWNLOAD_CONTENTID_NAMESPACE, id != null ? id.namespace : "");
        intent.putExtra(EXTRA_IS_OFF_THE_RECORD, isOffTheRecord);
        return intent;
    }
}
