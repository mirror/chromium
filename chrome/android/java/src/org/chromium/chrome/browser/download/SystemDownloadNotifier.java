// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;

import org.chromium.base.VisibleForTesting;
import org.chromium.content.browser.DownloadInfo;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import javax.annotation.Nullable;

/**
 * DownloadNotifier implementation that creates and updates download notifications.
 * This class creates the {@link DownloadNotificationService} when needed, and binds
 * to the latter to issue calls to show and update notifications.
 */
public class SystemDownloadNotifier implements DownloadNotifier {
    private static final int DOWNLOAD_NOTIFICATION_TYPE_PROGRESS = 0;
    private static final int DOWNLOAD_NOTIFICATION_TYPE_SUCCESS = 1;
    private static final int DOWNLOAD_NOTIFICATION_TYPE_FAILURE = 2;
    private static final int DOWNLOAD_NOTIFICATION_TYPE_CANCEL = 3;
    private static final int DOWNLOAD_NOTIFICATION_TYPE_CLEAR = 4;
    private static final int DOWNLOAD_NOTIFICATION_TYPE_PAUSE = 5;
    private final Context mApplicationContext;
    private final Object mLock = new Object();
    @Nullable private DownloadNotificationService mBoundService;
    private boolean mServiceStarted;
    private Set<Integer> mActiveNotificationIds = new HashSet<Integer>();
    private List<PendingNotificationInfo> mPendingNotifications =
            new ArrayList<PendingNotificationInfo>();

    /**
     * Pending download notifications to be posted.
     */
    private static class PendingNotificationInfo {
        // Pending download notifications to be posted.
        public final int type;
        public final DownloadInfo downloadInfo;
        public final Intent intent;
        public final long startTime;
        public final boolean isAutoResumable;

        public PendingNotificationInfo(int type, DownloadInfo downloadInfo, Intent intent,
                long startTime, boolean isAutoResumable) {
            this.type = type;
            this.downloadInfo = downloadInfo;
            this.intent = intent;
            this.startTime = startTime;
            this.isAutoResumable = isAutoResumable;
        }
    }

    /**
     * Constructor.
     * @param context Application context.
     */
    public SystemDownloadNotifier(Context context) {
        mApplicationContext = context.getApplicationContext();
    }

    /**
     * Object to receive information as the service is started and stopped.
     */
    private final ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            synchronized (mLock) {
                mBoundService = ((DownloadNotificationService.LocalBinder) service).getService();
                // updateDownloadNotification() may leave some outstanding notifications
                // before the service is connected, handle them now.
                handlePendingNotifications();
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            synchronized (mLock) {
                mBoundService = null;
                mServiceStarted = false;
            }
        }
    };

    /**
     * For tests only: sets the DownloadNotificationService.
     * @param service An instance of DownloadNotificationService.
     */
    @VisibleForTesting
    void setDownloadNotificationService(DownloadNotificationService service) {
        synchronized (mLock) {
            mBoundService = service;
        }
    }

    /**
     * Handles all the pending notifications that hasn't been processed.
     */
    @VisibleForTesting
    void handlePendingNotifications() {
        synchronized (mLock) {
            if (mPendingNotifications.isEmpty()) return;
            for (PendingNotificationInfo info : mPendingNotifications) {
                switch (info.type) {
                    case DOWNLOAD_NOTIFICATION_TYPE_PROGRESS:
                        notifyDownloadProgress(info.downloadInfo, info.startTime);
                        break;
                    case DOWNLOAD_NOTIFICATION_TYPE_SUCCESS:
                        notifyDownloadSuccessful(info.downloadInfo, info.intent);
                        break;
                    case DOWNLOAD_NOTIFICATION_TYPE_FAILURE:
                        notifyDownloadFailed(info.downloadInfo);
                        break;
                    case DOWNLOAD_NOTIFICATION_TYPE_CANCEL:
                        cancelNotification(info.downloadInfo.getNotificationId());
                        break;
                    case DOWNLOAD_NOTIFICATION_TYPE_CLEAR:
                        clearPendingDownloads();
                        break;
                    default:
                        assert false;
                }
            }
            mPendingNotifications.clear();
        }
    }

    /**
     * Starts and binds to the download notification service if needed.
     */
    private void startAndBindToServiceIfNeeded() {
        assert Thread.holdsLock(mLock);
        if (mServiceStarted) return;
        startService();
        mServiceStarted = true;
    }

    /**
     * Stops the download notification service if there are no download in progress.
     */
    private void stopServiceIfNeeded() {
        assert Thread.holdsLock(mLock);
        if (mActiveNotificationIds.isEmpty() && mServiceStarted) {
            stopService();
            mServiceStarted = false;
        }
    }

    /**
     * Starts and binds to the download notification service.
     */
    @VisibleForTesting
    void startService() {
        assert Thread.holdsLock(mLock);
        mApplicationContext.startService(
                new Intent(mApplicationContext, DownloadNotificationService.class));
        mApplicationContext.bindService(new Intent(mApplicationContext,
                DownloadNotificationService.class), mConnection, Context.BIND_AUTO_CREATE);
    }

    /**
     * Stops the download notification service.
     */
    @VisibleForTesting
    void stopService() {
        assert Thread.holdsLock(mLock);
        mApplicationContext.stopService(
                new Intent(mApplicationContext, DownloadNotificationService.class));
    }

    @Override
    public void cancelNotification(int notificationId) {
        DownloadInfo info = new DownloadInfo.Builder().setNotificationId(notificationId).build();
        updateDownloadNotification(DOWNLOAD_NOTIFICATION_TYPE_CANCEL, info, null, -1, false);
    }

    @Override
    public void notifyDownloadSuccessful(DownloadInfo downloadInfo, Intent intent) {
        updateDownloadNotification(
                DOWNLOAD_NOTIFICATION_TYPE_SUCCESS, downloadInfo, intent, -1, false);
    }

    @Override
    public void notifyDownloadFailed(DownloadInfo downloadInfo) {
        updateDownloadNotification(
                DOWNLOAD_NOTIFICATION_TYPE_FAILURE, downloadInfo, null, -1, false);
    }

    @Override
    public void notifyDownloadProgress(DownloadInfo downloadInfo, long startTime) {
        updateDownloadNotification(
                DOWNLOAD_NOTIFICATION_TYPE_PROGRESS, downloadInfo, null, startTime, false);
    }

    @Override
    public void notifyDownloadPaused(DownloadInfo downloadInfo, boolean isAutoResumable) {
        updateDownloadNotification(
                DOWNLOAD_NOTIFICATION_TYPE_PAUSE, downloadInfo, null, -1, isAutoResumable);
    }

    @Override
    public void clearPendingDownloads() {
        updateDownloadNotification(DOWNLOAD_NOTIFICATION_TYPE_CLEAR, null, null, -1, false);
    }

    /**
     * Updates the download notification if the notification service is started. Otherwise,
     * wait for the notification service to become ready.
     * @param type Type of the notification.
     * @param info Download information.
     * @param intent Action to perform when clicking on the notification.
     * @param startTime Download start time.
     * @param isAutoResumable Whether download can be auto resumed when network becomes available.
     */
    private void updateDownloadNotification(
            int type, DownloadInfo info, Intent intent, long startTime, boolean isAutoResumable) {
        synchronized (mLock) {
            startAndBindToServiceIfNeeded();
            if (type == DOWNLOAD_NOTIFICATION_TYPE_PROGRESS) {
                mActiveNotificationIds.add(info.getNotificationId());
            } else if (type != DOWNLOAD_NOTIFICATION_TYPE_CLEAR) {
                mActiveNotificationIds.remove(info.getNotificationId());
            }
            if (mBoundService == null) {
                // We need to wait for the service to connect before we can handle
                // the notification. Put the notification in the pending notifications
                // list.
                mPendingNotifications.add(new PendingNotificationInfo(
                        type, info, intent, startTime, isAutoResumable));
            } else {
                switch (type) {
                    case DOWNLOAD_NOTIFICATION_TYPE_PROGRESS:

                        mBoundService.notifyDownloadProgress(info.getNotificationId(),
                                info.getDownloadGuid(), info.getFileName(),
                                info.getPercentCompleted(), info.getTimeRemainingInMillis(),
                                startTime, info.isResumable());
                        break;
                    case DOWNLOAD_NOTIFICATION_TYPE_PAUSE:
                        assert info.isResumable();
                        mBoundService.notifyDownloadPaused(info.getNotificationId(),
                                info.getDownloadGuid(), info.getFileName(),
                                info.isResumable(), isAutoResumable);
                        break;
                    case DOWNLOAD_NOTIFICATION_TYPE_SUCCESS:
                        mBoundService.notifyDownloadSuccessful(
                                info.getNotificationId(), info.getFileName(), intent);
                        stopServiceIfNeeded();
                        break;
                    case DOWNLOAD_NOTIFICATION_TYPE_FAILURE:
                        mBoundService.notifyDownloadFailed(
                                info.getNotificationId(), info.getFileName());
                        stopServiceIfNeeded();
                        break;
                    case DOWNLOAD_NOTIFICATION_TYPE_CANCEL:
                        mBoundService.cancelNotification(info.getNotificationId());
                        stopServiceIfNeeded();
                        break;
                    case DOWNLOAD_NOTIFICATION_TYPE_CLEAR:
                        mBoundService.clearPendingDownloads();
                        stopServiceIfNeeded();
                        break;
                    default:
                        assert false;
                }
            }
        }
    }
}
