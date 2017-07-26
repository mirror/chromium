// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static org.chromium.chrome.browser.download.DownloadSnackbarController.INVALID_NOTIFICATION_ID;

import android.app.Notification;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;

import java.util.HashMap;
import java.util.Map;

/**
 * Manager to stop and start the foreground service associated with downloads.
 */
public class DownloadForegroundServiceManager {
    public enum DownloadStatus { START, PAUSE, RESUME, CANCEL, COMPLETE }

    private static final String TAG = "DownloadFgSManager";

    private final Map<Integer, Notification> mActiveNotifications = new HashMap<>();

    private int mPinnedNotificationId = INVALID_NOTIFICATION_ID;
    private Notification mPinnedNotification;

    // This is true when context.bindService has been called and before context.unbindService.
    private boolean mIsServiceBound;
    // This is non-null when onServiceConnected has been called (aka service has been initialized).
    private DownloadForegroundService mBoundService;

    public DownloadForegroundServiceManager() {}

    /**
     * Called by DownloadNotificationStore when there is a change in the list of downloads.
     * @param downloadStatus whether the download is started, paused, resumed, canceled, completed.
     * @param notificationId identifier of the notification associated with the download.
     * @param notification the actual notification associated with the download.
     */
    public void updateDownloadStatus(Context context, DownloadStatus downloadStatus,
            int notificationId, Notification notification) {
        boolean isActive =
                downloadStatus == DownloadStatus.START || downloadStatus == DownloadStatus.RESUME;

        if (isActive) {
            mActiveNotifications.put(notificationId, notification);
            // If no notifications were pinned originally, start service with this one.
            if (mPinnedNotificationId == INVALID_NOTIFICATION_ID) {
                mPinnedNotificationId = notificationId;
                mPinnedNotification = notification;
                startDownloadForegroundService(context, notificationId, notification);
            }
        } else {
            if (mActiveNotifications.containsKey(notificationId)) {
                mActiveNotifications.remove(notificationId);
                // If there are no active notifications left, stop service.
                if (mActiveNotifications.isEmpty()) {
                    mPinnedNotificationId = INVALID_NOTIFICATION_ID;
                    mPinnedNotification = null;
                    stopDownloadForegroundService(downloadStatus == DownloadStatus.COMPLETE);
                    return;
                }
                // If there are active notifications, use another one instead.
                if (mPinnedNotificationId == notificationId) {
                    Map.Entry<Integer, Notification> next =
                            mActiveNotifications.entrySet().iterator().next();
                    mPinnedNotificationId = next.getKey();
                    mPinnedNotification = next.getValue();
                    startDownloadForegroundService(context, next.getKey(), next.getValue());
                }
            }
        }
    }

    @VisibleForTesting
    void startDownloadForegroundService(
            Context context, int notificationId, Notification notification) {
        if (mIsServiceBound) {
            updatePinnedNotificationIfNeeded(notificationId, notification);
        } else {
            mIsServiceBound = true;
            bindService(context);
        }
    }

    @VisibleForTesting
    void stopDownloadForegroundService(boolean isComplete) {
        if (mIsServiceBound && mBoundService != null) {
            mPinnedNotificationId = INVALID_NOTIFICATION_ID;
            mPinnedNotification = null;
            mIsServiceBound = false;

            unbindService();
            stopForegroundService(isComplete);
        }
    }

    private final ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            if (!(service instanceof DownloadForegroundService.LocalBinder)) {
                Log.w(TAG,
                        "Not from DownloadNotificationService, do not connect."
                                + " Component name: " + className);
                return;
            }
            mBoundService = ((DownloadForegroundService.LocalBinder) service).getService();
            handlePendingNotifications();
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            mBoundService = null;
        }
    };

    @VisibleForTesting
    void bindService(Context context) {
        DownloadForegroundService.startDownloadForegroundService(context);
        ContextUtils.getApplicationContext().bindService(
                new Intent(ContextUtils.getApplicationContext(), DownloadForegroundService.class),
                mConnection, Context.BIND_AUTO_CREATE);
    }

    @VisibleForTesting
    void unbindService() {
        if (mBoundService != null) {
            ContextUtils.getApplicationContext().unbindService(mConnection);
            mIsServiceBound = false;
        }
    }

    @VisibleForTesting
    void stopForegroundService(boolean isComplete) {
        if (mBoundService != null) {
            mBoundService.stopDownloadForegroundService(isComplete);
        }
    }

    @VisibleForTesting
    void updatePinnedNotificationIfNeeded(int notificationId, Notification notification) {
        if (mBoundService != null && notificationId != INVALID_NOTIFICATION_ID
                && notification != null) {
            mBoundService.updatePinnedNotification(notificationId, notification);
        }
    }

    /**
     * Using stored notificationId and notification, check if anything happened during startup.
     */
    @VisibleForTesting
    void handlePendingNotifications() {
        // If stop was called, don't even start foreground.
        if (mPinnedNotificationId == INVALID_NOTIFICATION_ID && mPinnedNotification == null) {
            unbindService();
            return;
        }
        // Start the foreground with stored notificationId and notification.
        updatePinnedNotificationIfNeeded(mPinnedNotificationId, mPinnedNotification);
    }

    @VisibleForTesting
    void setBoundService(DownloadForegroundService service) {
        mIsServiceBound = true;
        mBoundService = service;
    }
}
