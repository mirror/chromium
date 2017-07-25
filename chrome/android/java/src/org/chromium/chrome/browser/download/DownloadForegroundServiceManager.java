// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static org.chromium.chrome.browser.notifications.NotificationConstants.DEFAULT_NOTIFICATION_ID;

import android.app.Notification;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;

import java.util.HashMap;
import java.util.Map;

/**
 * Manager to stop and start the foreground service associated with downloads.
 */
public class DownloadForegroundServiceManager {
    public enum DownloadStatus { START, PAUSE, RESUME, CANCEL, COMPLETE }

    static final String EXTRA_NOTIFICATION =
            "org.chromium.chrome.browser.download.DownloadServiceManager.EXTRA_NOTIFICATION";
    static final String EXTRA_NOTIFICATION_ID =
            "org.chromium.chrome.browser.download.DownloadServiceManager.EXTRA_NOTIFICATION_ID";

    private static final String TAG = "DownloadFSM";

    private final Context mApplicationContext;
    private final Map<Integer, Notification> mActiveNotifications = new HashMap<>();

    private int mPinnedNotificationId = DEFAULT_NOTIFICATION_ID;
    private Notification mPinnedNotification;
    private boolean mIsServiceBound = false;
    private DownloadForegroundService mBoundService;
    private boolean mQueuedStopIsComplete;

    public DownloadForegroundServiceManager(Context context) {
        mApplicationContext = context.getApplicationContext();
    }

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
            if (mPinnedNotificationId == DEFAULT_NOTIFICATION_ID) {
                mPinnedNotificationId = notificationId;
                mPinnedNotification = notification;
                startDownloadForegroundService(context, notificationId, notification);
            }
        } else {
            if (mActiveNotifications.containsKey(notificationId)) {
                mActiveNotifications.remove(notificationId);
                if (mActiveNotifications.isEmpty()) {
                    mPinnedNotificationId = DEFAULT_NOTIFICATION_ID;
                    mPinnedNotification = null;
                    stopDownloadForegroundService(downloadStatus == DownloadStatus.COMPLETE);
                    return;
                }
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
        if (!mIsServiceBound) {
            mIsServiceBound = true;
            bindService(context, notificationId, notification);
        } else {
            updatePinnedNotificationIfNeeded(notificationId, notification);
        }
    }

    @VisibleForTesting
    void stopDownloadForegroundService(boolean isComplete) {
        if (mIsServiceBound && mBoundService != null) {
            mPinnedNotificationId = DEFAULT_NOTIFICATION_ID;
            mPinnedNotification = null;
            mIsServiceBound = false;

            unbindService(isComplete);
        } else {
            mQueuedStopIsComplete = isComplete;
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
    void bindService(Context context, int notificationId, Notification notification) {
        Intent intent = new Intent();
        intent.putExtra(EXTRA_NOTIFICATION, notification);
        intent.putExtra(EXTRA_NOTIFICATION_ID, notificationId);

        DownloadForegroundService.startDownloadForegroundService(context, intent);
        mApplicationContext.bindService(
                new Intent(mApplicationContext, DownloadForegroundService.class), mConnection,
                Context.BIND_AUTO_CREATE);
    }

    @VisibleForTesting
    void unbindService(boolean isComplete) {
        if (mBoundService != null) {
            mBoundService.stopDownloadForegroundService(isComplete);
        }

        mApplicationContext.unbindService(mConnection);
        mIsServiceBound = false;
    }

    @VisibleForTesting
    void updatePinnedNotificationIfNeeded(int notificationId, Notification notification) {
        if (mBoundService != null) {
            mBoundService.updatePinnedNotificationIfNeeded(notificationId, notification);
        }
    }

    @VisibleForTesting
    void handlePendingNotifications() {
        // Check if things have changed while the service was starting.
        if (mPinnedNotificationId == DEFAULT_NOTIFICATION_ID & mPinnedNotification == null) {
            stopDownloadForegroundService(mQueuedStopIsComplete /* isComplete */);
            return;
        }
        updatePinnedNotificationIfNeeded(mPinnedNotificationId, mPinnedNotification);
    }

    @VisibleForTesting
    void setBoundService(DownloadForegroundService service) {
        mIsServiceBound = true;
        mBoundService = service;
    }
}
