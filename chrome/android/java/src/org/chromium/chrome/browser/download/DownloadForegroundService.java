// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.app.Notification;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.support.annotation.Nullable;

import org.chromium.base.Log;
import org.chromium.chrome.browser.AppHooks;

/**
 * Keep-alive foreground service for downloads.
 */
public class DownloadForegroundService extends Service {
    private final IBinder mBinder = new LocalBinder();

    /**
     * Start the foreground service with this given context.
     * @param context The context used to start service.
     */
    public static void startDownloadForegroundService(Context context) {
        Log.e("joy", "startDownloadForegroundService");
        AppHooks.get().startForegroundService(new Intent(context, DownloadForegroundService.class));
    }

    /**
     * Update the foreground service to be pinned to a different notification.
     * @param notificationId The id of the new notification to be pinned to.
     * @param notification The new notification to be pinned to.
     */
    public void startOrUpdateForegroundService(int notificationId, Notification notification) {
        // TODO(jming): Make sure there is not weird UI in switching the pinned notification.
        Log.e("joy", "startOrUpdateForegroundService: " + notificationId + "," + notification);
//        NotificationManager notificationManager =
//                (NotificationManager) ContextUtils.getApplicationContext()
//                        .getSystemService(Context.NOTIFICATION_SERVICE);
//        Log.e("joy", "DFS notification manager: " + notificationManager);
//        Notification notification2 =                 NotificationBuilderFactory
//                .createChromeNotificationBuilder(
//                        true /* preferCompat */, ChannelDefinitions.CHANNEL_ID_DOWNLOADS)
//                .setSmallIcon(org.chromium.chrome.R.drawable.ic_file_download_white_24dp)
//                .setContentTitle("fake content title")
//                .setContentText("fake content text")
//                .build();
//        notificationManager.notify(notificationId, notification2);
//        startForeground(notificationId + 1, notification2);
        startForeground(notificationId, notification);
    }

    /**
     * Stop the foreground service that is running.
     * @param isComplete If the download has been complete and, therefore, if its notification
     *                   should be killed.
     */
    public void stopDownloadForegroundService(boolean isComplete) {
        // TODO(jming): Check to make sure the notification does not get killed.
        stopForeground(isComplete /* killNotification */);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.e("joy", "onStartCommand");
        // This should restart service after Chrome gets killed (except for Android 4.4.2).
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        // TODO(jming): Check if
        super.onDestroy();
    }

    @Override
    public void onTaskRemoved(Intent rootIntent) {
        super.onTaskRemoved(rootIntent);
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
