// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static org.chromium.chrome.browser.download.DownloadNotificationService2.getNextNotificationId;
import static org.chromium.chrome.browser.download.DownloadSnackbarController.INVALID_NOTIFICATION_ID;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import android.support.annotation.Nullable;
import android.support.v4.app.ServiceCompat;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.AppHooks;

/**
 * Keep-alive foreground service for downloads.
 */
public class DownloadForegroundService extends Service {
    private static final String KEY_PINNED_NOTIFICATION_ID = "PinnedNotificationId";
    private final IBinder mBinder = new LocalBinder();

    /**
     * Start the foreground service with this given context.
     * @param context The context used to start service.
     */
    public static void startDownloadForegroundService(Context context) {
        // TODO(crbug.com/770389): Grab a WakeLock here until the service has started.
        AppHooks.get().startForegroundService(new Intent(context, DownloadForegroundService.class));
    }

    /**
     * Start the foreground service or update it to be pinned to a different notification.
     *
     * @param newNotificationId The ID of the new notification to pin the service to.
     * @param newNotification   The new notification to be pinned to the service.
     * @param oldNotificationId The ID of the original notification that was pinned to the service,
     *                          can be INVALID_NOTIFICATION_ID if the service is just starting.
     * @param oldNotification   The original notification the service was pinned to, in case an
     *                          adjustment needs to be made (in the case it could not be detached).
     */
    public void startOrUpdateForegroundService(int newNotificationId, Notification newNotification,
            int oldNotificationId, Notification oldNotification) {
        Log.e("joy", "startOrUpdateForegroundService");
        // Handle notifications and start foreground.
        if (Build.VERSION.SDK_INT >= 24) {
            Log.e("joy", "SDK_INT >= 24");
            // If possible, detach notification so it doesn't get cancelled by accident.
            stopForegroundInternal(STOP_FOREGROUND_DETACH);
            startForegroundInternal(newNotificationId, newNotification);
        } else {
            Log.e("joy", "SDK_INT < 24");
            // Otherwise start the foreground and relaunch the originally pinned notification.
            startForegroundInternal(newNotificationId, newNotification);
            relaunchOldNotification(oldNotificationId, oldNotification);
        }

        // Record when starting foreground and when updating pinned notification.
        int pinnedNotificationId = getPinnedNotificationId();
        if (pinnedNotificationId == INVALID_NOTIFICATION_ID) {
            DownloadNotificationUmaHelper.recordForegroundServiceLifecycleHistogram(
                    DownloadNotificationUmaHelper.ForegroundLifecycle.START);
        } else {
            if (pinnedNotificationId != newNotificationId) {
                DownloadNotificationUmaHelper.recordForegroundServiceLifecycleHistogram(
                        DownloadNotificationUmaHelper.ForegroundLifecycle.UPDATE);
            }
        }
        updatePinnedNotificationId(newNotificationId);
    }

    /**
     * Stop the foreground service that is running.
     *
     * @param detachNotification Whether to try to detach the notification from the service.
     * @param killNotification   Whether to kill the notification with the service.
     * @return                   Whether the notification was handled properly (ie. it was detached
     *                           or killed as intended). If this returns false, the notification id
     *                           will be persisted in shared preferences to be handled later.
     */
    public boolean stopDownloadForegroundService(boolean detachNotification,
            boolean killNotification, int pinnedNotificationId, Notification pinnedNotification) {
        Log.e("joy", "stopDownloadForegroundService");
        // Record when stopping foreground.
        DownloadNotificationUmaHelper.recordForegroundServiceLifecycleHistogram(
                DownloadNotificationUmaHelper.ForegroundLifecycle.STOP);
        DownloadNotificationUmaHelper.recordServiceStoppedHistogram(
                DownloadNotificationUmaHelper.ServiceStopped.STOPPED, true /* withForeground */);

        // Handle notifications and stop foreground.
        boolean notificationHandledProperly;
        if (Build.VERSION.SDK_INT >= 24) {
            Log.e("joy", "SDK_INT >= 24");
            // Android N+ has the option to detach notifications from the service, so detach or kill
            // the notification as needed when stopping service.
            int flag = (detachNotification) ? STOP_FOREGROUND_DETACH : STOP_FOREGROUND_REMOVE;
            stopForegroundInternal(flag);
            notificationHandledProperly = true;

        } else if (Build.VERSION.SDK_INT >= 23) {
            Log.e("joy", "SDK_INT >= 23");
            // Android M+ can't detach the notification but doesn't have other caveats. Kill the
            // notification and relaunch if detach was desired.
            stopForegroundInternal(killNotification);
            if (detachNotification && killNotification) {
                relaunchOldNotification(pinnedNotificationId, pinnedNotification);
            }

            // Notification is handled properly if killed or relaunched.
            notificationHandledProperly = !detachNotification || killNotification;

        } else if (Build.VERSION.SDK_INT >= 21) {
            Log.e("joy", "SDK_INT >= 21");
            // In phones that are Lollipop and older (API < 23), the service gets killed with the
            // task, which might result in the notification being unable to be relaunched where it
            // needs to be. Relaunch the old notification before stopping the service.
            if (detachNotification && killNotification) {
                relaunchOldNotification(getNextNotificationId(), pinnedNotification);
            }
            stopForegroundInternal(killNotification);

            // Notification is handled properly if killed or relaunched.
            notificationHandledProperly = !detachNotification || killNotification;

        } else {
            Log.e("joy", "SDK_INT < 21");
            // For pre-Lollipop phones (API < 21), we need to kill all notification because
            // otherwise the notification gets stuck in the ongoing state.
            relaunchOldNotification(getNextNotificationId(), pinnedNotification);
            stopForegroundInternal(true);
            notificationHandledProperly = true;
        }

        // If the notification was handled properly (ie. killed or detached), clear observer and
        // stored pinned ID. Otherwise, continue persisting this information in case it is needed.
        if (notificationHandledProperly) {
            DownloadForegroundServiceObservers.removeObserver(
                    DownloadNotificationServiceObserver.class);
            clearPinnedNotificationId();
        }
        Log.e("joy", "notificationHandledProperly: " + notificationHandledProperly);

        return notificationHandledProperly;
    }

    private void relaunchOldNotification(int notificationId, Notification notification) {
        Log.e("joy", "relaunchOldNotification notificationId: " + 99999);
        if (notificationId != INVALID_NOTIFICATION_ID && notification != null) {
            NotificationManager notificationManager =
                    (NotificationManager) ContextUtils.getApplicationContext().getSystemService(
                            Context.NOTIFICATION_SERVICE);
            Log.e("joy", "notifying");
            notificationManager.notify(99999, notification);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // In the case the service was restarted when the intent is null.
        if (intent == null) {
            DownloadNotificationUmaHelper.recordServiceStoppedHistogram(
                    DownloadNotificationUmaHelper.ServiceStopped.START_STICKY, true);

            // Alert observers that the service restarted with null intent.
            // Pass along the id of the notification that was pinned to the service when it died so
            // that the observers can do any corrections (ie. relaunch notification) if needed.
            DownloadForegroundServiceObservers.alertObserversServiceRestarted(
                    getPinnedNotificationId());

            // Allow observers to restart service on their own, if needed.
            stopSelf();
        }

        // This should restart service after Chrome gets killed (except for Android 4.4.2).
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        DownloadNotificationUmaHelper.recordServiceStoppedHistogram(
                DownloadNotificationUmaHelper.ServiceStopped.DESTROYED, true /* withForeground */);
        DownloadForegroundServiceObservers.alertObserversServiceDestroyed();
        super.onDestroy();
    }

    @Override
    public void onTaskRemoved(Intent rootIntent) {
        DownloadNotificationUmaHelper.recordServiceStoppedHistogram(
                DownloadNotificationUmaHelper.ServiceStopped.TASK_REMOVED, true /*withForeground*/);
        DownloadForegroundServiceObservers.alertObserversTaskRemoved();
        super.onTaskRemoved(rootIntent);
    }

    @Override
    public void onLowMemory() {
        DownloadNotificationUmaHelper.recordServiceStoppedHistogram(
                DownloadNotificationUmaHelper.ServiceStopped.LOW_MEMORY, true /* withForeground */);
        super.onLowMemory();
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    /**
     * Class for clients to access.
     */
    class LocalBinder extends Binder {
        DownloadForegroundService getService() {
            return DownloadForegroundService.this;
        }
    }

    /**
     * Get stored value for the id of the notification pinned to the service.
     * This has to be persisted in the case that the service dies and the notification dies with it.
     * @return Id of the notification pinned to the service.
     */
    @VisibleForTesting
    static int getPinnedNotificationId() {
        SharedPreferences sharedPrefs = ContextUtils.getAppSharedPreferences();
        return sharedPrefs.getInt(KEY_PINNED_NOTIFICATION_ID, INVALID_NOTIFICATION_ID);
    }

    /**
     * Set stored value for the id of the notification pinned to the service.
     * This has to be persisted in the case that the service dies and the notification dies with it.
     * @param pinnedNotificationId Id of the notification pinned to the service.
     */
    private static void updatePinnedNotificationId(int pinnedNotificationId) {
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putInt(KEY_PINNED_NOTIFICATION_ID, pinnedNotificationId)
                .apply();
    }

    /**
     * Clear stored value for the id of the notification pinned to the service.
     */
    @VisibleForTesting
    static void clearPinnedNotificationId() {
        ContextUtils.getAppSharedPreferences().edit().remove(KEY_PINNED_NOTIFICATION_ID).apply();
    }

    /** Methods for testing. */

    @VisibleForTesting
    void startForegroundInternal(int notificationId, Notification notification) {
        Log.e("joy", "startForegroundInternal notificationId: " + notificationId);
        startForeground(notificationId, notification);
    }

    @VisibleForTesting
    void stopForegroundInternal(int flags) {
        Log.e("joy", "stopForegroundInternal flags: " + flags);
        ServiceCompat.stopForeground(this, flags);
    }

    @VisibleForTesting
    void stopForegroundInternal(boolean removeNotification) {
        Log.e("joy", "stopForegroundInternal removeNotification: " + removeNotification);
        stopForeground(removeNotification);
    }

    @VisibleForTesting
    boolean isSdkAtLeast24() {
        return Build.VERSION.SDK_INT >= 24;
    }
}
