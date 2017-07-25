// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static org.chromium.chrome.browser.download.DownloadForegroundServiceManager.EXTRA_NOTIFICATION;
import static org.chromium.chrome.browser.download.DownloadForegroundServiceManager.EXTRA_NOTIFICATION_ID;
import static org.chromium.chrome.browser.notifications.NotificationConstants.DEFAULT_NOTIFICATION_ID;

import android.app.Notification;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.support.annotation.Nullable;

/**
 * Keep-alive foreground service for downloads.
 */
public class DownloadForegroundService extends Service {
    private final IBinder mBinder = new LocalBinder();

    private int mPinnedNotificationId = DEFAULT_NOTIFICATION_ID;
    private Notification mPinnedNotification;

    /**
     * Start the foreground service with this given context and intent.
     * @param context The context used to start service.
     * @param source The intent that should be used to start the service (should include
     *               notificationId and notification as extras).
     */
    public static void startDownloadForegroundService(Context context, Intent source) {
        Intent intent = source != null ? new Intent(source) : new Intent();
        intent.setComponent(new ComponentName(context, DownloadForegroundService.class));

        context.startService(intent);
    }

    /**
     * Update the foreground service to be pinned to a different notification.
     * @param notificationId The id of the new notification to be pinned to.
     * @param notification The new notification to be pinned to.
     */
    public void updatePinnedNotificationIfNeeded(int notificationId, Notification notification) {
        if (notificationId != mPinnedNotificationId && notificationId != DEFAULT_NOTIFICATION_ID
                && notification != mPinnedNotification && notification != null) {
            mPinnedNotificationId = notificationId;
            mPinnedNotification = notification;

            // TODO(jming): Figure out if this results in weird UI.
            startForeground(mPinnedNotificationId, mPinnedNotification);
        }
    }

    public void stopDownloadForegroundService(boolean isComplete) {
        // TODO(jming): Check to make sure the notification does not get killed.
        stopForeground(isComplete /* killNotification */);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // Parse intent for notification information.
        if (intent.hasExtra(EXTRA_NOTIFICATION) && intent.hasExtra(EXTRA_NOTIFICATION_ID)) {
            mPinnedNotificationId =
                    intent.getIntExtra(EXTRA_NOTIFICATION_ID, DEFAULT_NOTIFICATION_ID);
            mPinnedNotification = intent.getParcelableExtra(EXTRA_NOTIFICATION);

            if (mPinnedNotificationId != DEFAULT_NOTIFICATION_ID && mPinnedNotification != null) {
                startForeground(mPinnedNotificationId, mPinnedNotification);
            }
        }
        // This should restart service after Chrome gets killed (except for Android 4.4.2).
        return START_STICKY;
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    /**
     * Class for clients to access.
     */
    public class LocalBinder extends Binder {
        DownloadForegroundService getService() {
            return DownloadForegroundService.this;
        }
    }
}
