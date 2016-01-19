// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.support.v4.app.NotificationCompat;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeApplication;
import org.chromium.chrome.browser.init.BrowserParts;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.init.EmptyBrowserParts;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.ui.base.LocalizationUtils;

import java.text.NumberFormat;
import java.util.List;
import java.util.Locale;

/**
 * Service responsible for creating and updating download notifications even after
 * Chrome gets killed.
 */
public class DownloadNotificationService extends Service {
    static final String EXTRA_DOWNLOAD_ID = "DownloadId";
    static final String EXTRA_DOWNLOAD_FILE_NAME = "DownloadFileName";
    static final String ACTION_DOWNLOAD_RESUME =
            "org.chromium.chrome.browser.download.DOWNLOAD_RESUME";
    static final int INVALID_DOWNLOAD_PERCENTAGE = -1;
    private static final String NOTIFICATION_NAMESPACE = "DownloadNotificationService";
    private static final String TAG = "DownloadNotification";
    private final IBinder mBinder = new LocalBinder();
    private NotificationManager mNotificationManager;
    private Context mContext;

    /**
     * Class for clients to access.
     */
    public class LocalBinder extends Binder {
        DownloadNotificationService getService() {
            return DownloadNotificationService.this;
        }
    }

    @Override
    public void onTaskRemoved(Intent rootIntent) {
        // This funcion is called when Chrome is swiped away from the recent apps
        // drawer. So it doesn't catch all scenarios that chrome can get killed.
        // This will only help Android 4.4.2.
        pauseAllDownloads();
        stopSelf();
    }

    @Override
    public void onCreate() {
        mContext = getApplicationContext();
        mNotificationManager = (NotificationManager) mContext.getSystemService(
                Context.NOTIFICATION_SERVICE);

        // Because this service is a started service and returns START_STICKY in
        // onStartCommand(), it will be restarted as soon as resources are available
        // after it is killed. As a result, onCreate() may be called after Chrome
        // gets killed and before user restarts chrome. In that case,
        // DownloadManagerService.hasDownloadManagerService() will return false as
        // there are no calls to initialize DownloadManagerService. Pause all the
        // download notifications as download will not progress without chrome.
        if (!DownloadManagerService.hasDownloadManagerService()) {
            pauseAllDownloads();
            stopSelf();
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (isDownloadResumptionIntent(intent)) {
            final int downloadId = IntentUtils.safeGetIntExtra(
                    intent, DownloadNotificationService.EXTRA_DOWNLOAD_ID, -1);
            final String fileName = IntentUtils.safeGetStringExtra(
                    intent, DownloadNotificationService.EXTRA_DOWNLOAD_FILE_NAME);
            BrowserParts parts = new EmptyBrowserParts() {
                @Override
                public void finishNativeInitialization() {
                    DownloadManagerService service =
                            DownloadManagerService.getDownloadManagerService(
                                    getApplicationContext());
                    service.resumeDownload(downloadId, fileName);
                }
            };
            try {
                ChromeBrowserInitializer.getInstance(mContext).handlePreNativeStartup(parts);
                ChromeBrowserInitializer.getInstance(mContext).handlePostNativeStartup(true, parts);
            } catch (ProcessInitException e) {
                Log.e(TAG, "Unable to load native library.", e);
                ChromeApplication.reportStartupErrorAndExit(e);
            }
        }

        // This should restart the service after Chrome gets killed. However, this
        // doesn't work on Android 4.4.2.
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    /**
     * Add a in-progress download notification.
     * @param downloadId ID of the download.
     * @param fileName File name of the download.
     * @param percentage Percentage completed. Value should be between 0 to 100 if
     *        the percentage can be determined, or -1 if it is unknown.
     * @param timeRemainingInMillis Remaining download time in milliseconds.
     * @param startTime Time when download started.
     */
    public void notifyDownloadProgress(
            int downloadId, String fileName, int percentage, long timeRemainingInMillis,
            long startTime) {
        boolean indeterminate = percentage == INVALID_DOWNLOAD_PERCENTAGE;
        NotificationCompat.Builder builder = buildNotification(
                android.R.drawable.stat_sys_download, fileName, null);
        builder.setOngoing(true).setProgress(100, percentage, indeterminate);
        if (!indeterminate) {
            NumberFormat formatter = NumberFormat.getPercentInstance(Locale.getDefault());
            String percentText = formatter.format(percentage / 100.0);
            String duration = LocalizationUtils.getDurationString(timeRemainingInMillis);
            builder.setContentText(duration).setContentInfo(percentText);
        }
        if (startTime > 0) builder.setWhen(startTime);
        updateNotification(downloadId, builder.build());
    }

    /**
     * Cancel a download notification.
     * @param downloadId ID of the download.
     */
    public void cancelNotification(int downloadId) {
        mNotificationManager.cancel(NOTIFICATION_NAMESPACE, downloadId);
    }

    /**
     * Change a download notification to paused state.
     * @param downloadId ID of the download.
     * @param fileName File name of the download.
     * @param isResumable whether download is resumable.
     */
    public void notifyDownloadPaused(int downloadId, String fileName, boolean isResumable) {
        NotificationCompat.Builder builder = buildNotification(
                android.R.drawable.ic_media_pause,
                fileName,
                mContext.getResources().getString(R.string.download_notification_paused));
        if (isResumable) {
            ComponentName component = new ComponentName(
                    mContext.getPackageName(), DownloadBroadcastReceiver.class.getName());
            Intent intent = new Intent(ACTION_DOWNLOAD_RESUME);
            intent.setComponent(component);
            intent.putExtra(EXTRA_DOWNLOAD_ID, downloadId);
            intent.putExtra(EXTRA_DOWNLOAD_FILE_NAME, fileName);
            builder.addAction(android.R.drawable.stat_sys_download_done,
                    mContext.getResources().getString(R.string.download_notification_resume_button),
                    PendingIntent.getBroadcast(
                            mContext, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT));
        }
        updateNotification(downloadId, builder.build());
    }

    /**
     * Add a download successful notification.
     * @param downloadId ID of the download.
     * @param fileName File name of the download.
     * @param intent Intent to launch when clicking the notification.
     */
    public void notifyDownloadSuccessful(int downloadId, String fileName, Intent intent) {
        NotificationCompat.Builder builder = buildNotification(
                android.R.drawable.stat_sys_download_done,
                fileName,
                mContext.getResources().getString(R.string.download_notification_completed));
        if (intent != null) {
            builder.setContentIntent(PendingIntent.getActivity(
                    mContext, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT));
        }
        updateNotification(downloadId, builder.build());
    }

    /**
     * Add a download failed notification.
     * @param downloadId ID of the download.
     * @param fileName File name of the download.
     * @param intent Intent to launch when clicking the notification.
     */
    public void notifyDownloadFailed(int downloadId, String fileName) {
        NotificationCompat.Builder builder = buildNotification(
                android.R.drawable.stat_sys_download_done,
                fileName,
                mContext.getResources().getString(R.string.download_notification_failed));
        updateNotification(downloadId, builder.build());
    }

    /**
     * Called to pause all the download notifications.
     */
    @VisibleForTesting
    void pauseAllDownloads() {
        SharedPreferences sharedPrefs =
                PreferenceManager.getDefaultSharedPreferences(mContext);
        List<DownloadManagerService.PendingNotification> notifications =
                DownloadManagerService.parseDownloadNotificationsFromSharedPrefs(sharedPrefs);
        for (DownloadManagerService.PendingNotification notification : notifications) {
            if (notification.downloadId > 0) {
                notifyDownloadPaused(
                        notification.downloadId, notification.fileName, notification.isResumable);
            }
        }
    }

    /**
     * Builds a notification to be displayed.
     * @param iconId Id of the notification icon.
     * @param title Title of the notification.
     * @param contentText Notification content text to be displayed.
     * @return notification builder that builds the notification to be displayed
     */
    private NotificationCompat.Builder buildNotification(
            int iconId, String title, String contentText) {
        NotificationCompat.Builder builder = new NotificationCompat.Builder(mContext)
                .setContentTitle(title)
                .setSmallIcon(iconId)
                .setLocalOnly(true)
                .setAutoCancel(true)
                .setContentText(contentText);
        return builder;
    }

    /**
     * Update the notification with id.
     * @param id Id of the notification that has to be updated.
     * @param notification the notification object that needs to be updated.
     */
    @VisibleForTesting
    void updateNotification(int id, Notification notification) {
        mNotificationManager.notify(NOTIFICATION_NAMESPACE, id, notification);
    }

    /**
     * Checks if an intent is about to resume a download.
     * @param intent An intent to validate.
     * @return true if the intent is for download resumption, or false otherwise.
     */
    static boolean isDownloadResumptionIntent(Intent intent) {
        if (!intent.hasExtra(DownloadNotificationService.EXTRA_DOWNLOAD_ID)
                || !intent.hasExtra(DownloadNotificationService.EXTRA_DOWNLOAD_FILE_NAME)) {
            return false;
        }
        final int downloadId = IntentUtils.safeGetIntExtra(
                intent, DownloadNotificationService.EXTRA_DOWNLOAD_ID, -1);
        if (downloadId == -1) return false;
        final String fileName = IntentUtils.safeGetStringExtra(
                intent, DownloadNotificationService.EXTRA_DOWNLOAD_FILE_NAME);
        if (fileName == null) return false;
        return true;
    }
}
