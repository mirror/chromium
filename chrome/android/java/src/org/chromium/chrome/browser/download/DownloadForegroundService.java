// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static org.chromium.chrome.browser.download.DownloadNotificationService.getResumptionAttemptLeft;

import android.app.Notification;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.support.annotation.Nullable;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.AppHooks;

import java.util.List;

/**
 * Keep-alive foreground service for downloads.
 */
public class DownloadForegroundService extends Service {
    private final IBinder mBinder = new LocalBinder();
    private final DownloadSharedPreferenceHelper mDownloadSharedPreferenceHelper =
            DownloadSharedPreferenceHelper.getInstance();

    /**
     * Start the foreground service with this given context.
     * @param context The context used to start service.
     */
    public static void startDownloadForegroundService(Context context) {
        AppHooks.get().startForegroundService(new Intent(context, DownloadForegroundService.class));
    }

    /**
     * Update the foreground service to be pinned to a different notification.
     * @param notificationId The id of the new notification to be pinned to.
     * @param notification The new notification to be pinned to.
     */
    public void startOrUpdateForegroundService(int notificationId, Notification notification) {
        // TODO(jming): Make sure there is not weird UI in switching the pinned notification.
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
        if (intent == null) {
            // Intent is null during process of restart because of START_STICKY.
            // TODO: Figure out if this is even needed? Shouldn't we just START_NOT_STICKY?
            DownloadNotificationService.getInstance().resumeAllPendingDownloads();
        }

        // This should restart service after Chrome gets killed (except for Android 4.4.2).
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        rescheduleDownloads();
        super.onDestroy();
    }

    // TODO: putting this here makes this class less generalized, is there another way to do this?
    private void rescheduleDownloads() {
        Context context = ContextUtils.getApplicationContext();

        // Cancel any existing task.  If we have any downloads to resume we'll reschedule another.
        DownloadResumptionScheduler.getDownloadResumptionScheduler(context).cancelTask();
        List<DownloadSharedPreferenceEntry> entries = mDownloadSharedPreferenceHelper.getEntries();
        if (entries.isEmpty()) return;

        boolean scheduleAutoResumption = false;
        boolean allowMeteredConnection = false;
        for (int i = 0; i < entries.size(); ++i) {
            DownloadSharedPreferenceEntry entry = entries.get(i);
            if (entry.isAutoResumable) {
                scheduleAutoResumption = true;
                if (entry.canDownloadWhileMetered) {
                    allowMeteredConnection = true;
                    break;
                }
            }
        }

        if (scheduleAutoResumption && getResumptionAttemptLeft() > 0) {
            DownloadResumptionScheduler.getDownloadResumptionScheduler(context).schedule(
                    allowMeteredConnection);
        }
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
