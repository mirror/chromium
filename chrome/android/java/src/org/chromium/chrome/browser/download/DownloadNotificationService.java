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
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Set;

/**
 * Service responsible for creating and updating download notifications even after
 * Chrome gets killed.
 */
public class DownloadNotificationService extends Service {
    static final String EXTRA_DOWNLOAD_NOTIFICATION_ID = "DownloadNotificationId";
    static final String EXTRA_DOWNLOAD_GUID = "DownloadGuid";
    static final String EXTRA_DOWNLOAD_FILE_NAME = "DownloadFileName";
    static final String ACTION_DOWNLOAD_CANCEL =
            "org.chromium.chrome.browser.download.DOWNLOAD_CANCEL";
    static final String ACTION_DOWNLOAD_PAUSE =
            "org.chromium.chrome.browser.download.DOWNLOAD_PAUSE";
    static final String ACTION_DOWNLOAD_RESUME =
            "org.chromium.chrome.browser.download.DOWNLOAD_RESUME";
    static final int INVALID_DOWNLOAD_PERCENTAGE = -1;
    @VisibleForTesting
    static final String PENDING_DOWNLOAD_NOTIFICATIONS = "PendingDownloadNotifications";
    private static final String NOTIFICATION_NAMESPACE = "DownloadNotificationService";
    private static final String TAG = "DownloadNotification";
    private final IBinder mBinder = new LocalBinder();
    private NotificationManager mNotificationManager;
    private SharedPreferences mSharedPrefs;
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
        mSharedPrefs = PreferenceManager.getDefaultSharedPreferences(mContext);

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
    public int onStartCommand(final Intent intent, int flags, int startId) {
        if (isDownloadOperationIntent(intent)) {
            handleDownloadOperation(intent);
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
     * @param notificationId Notification ID of the download.
     * @param downloadGuid GUID of the download.
     * @param fileName File name of the download.
     * @param percentage Percentage completed. Value should be between 0 to 100 if
     *        the percentage can be determined, or -1 if it is unknown.
     * @param timeRemainingInMillis Remaining download time in milliseconds.
     * @param startTime Time when download started.
     * @param isResumable whether the download can be resumed.
     */
    public void notifyDownloadProgress(int notificationId, String downloadGuid, String fileName,
            int percentage, long timeRemainingInMillis, long startTime, boolean isResumable) {
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
        builder.addAction(android.R.drawable.ic_menu_close_clear_cancel,
                mContext.getResources().getString(R.string.download_notification_cancel_button),
                buildPendingIntent(ACTION_DOWNLOAD_CANCEL, notificationId, downloadGuid, fileName));
        if (isResumable) {
            builder.addAction(android.R.drawable.ic_media_pause,
                    mContext.getResources().getString(R.string.download_notification_pause_button),
                    buildPendingIntent(ACTION_DOWNLOAD_PAUSE, notificationId, downloadGuid,
                            fileName));
        }
        updateNotification(notificationId, builder.build());
        addEntryToSharedPrefs(new DownloadSharedPreferenceEntry(
                notificationId, isResumable, true, downloadGuid, fileName));
    }

    /**
     * Cancel a download notification.
     * @param notificationId Notification ID of the download.
     */
    public void cancelNotification(int notificationId) {
        mNotificationManager.cancel(NOTIFICATION_NAMESPACE, notificationId);
        removeEntryFromSharedPrefs(notificationId);
    }

    /**
     * Change a download notification to paused state.
     * @param notificationId Notification ID of the download.
     * @param downloadGuid GUID of the download.
     * @param fileName File name of the download.
     * @param isResumable whether download is resumable.
     * @param isAutoResumable whether download is can be resumed automatically.
     */
    public void notifyDownloadPaused(int notificationId, String downloadGuid, String fileName,
            boolean isResumable, boolean isAutoResumable) {
        NotificationCompat.Builder builder = buildNotification(
                android.R.drawable.ic_media_pause,
                fileName,
                mContext.getResources().getString(R.string.download_notification_paused));
        PendingIntent cancelIntent =
                buildPendingIntent(ACTION_DOWNLOAD_CANCEL, notificationId, downloadGuid, fileName);
        builder.setDeleteIntent(cancelIntent);
        builder.addAction(android.R.drawable.ic_menu_close_clear_cancel,
                mContext.getResources().getString(R.string.download_notification_cancel_button),
                cancelIntent);
        if (isResumable) {
            builder.addAction(android.R.drawable.stat_sys_download_done,
                    mContext.getResources().getString(R.string.download_notification_resume_button),
                    buildPendingIntent(ACTION_DOWNLOAD_RESUME, notificationId, downloadGuid,
                            fileName));
        }
        updateNotification(notificationId, builder.build());
        // If download is not auto resumable, there is no need to keep it in SharedPreferences.
        if (!isResumable || !isAutoResumable) {
            removeEntryFromSharedPrefs(notificationId);
        }
    }

    /**
     * Add a download successful notification.
     * @param notificationId Notification ID of the download.
     * @param fileName File name of the download.
     * @param intent Intent to launch when clicking the notification.
     */
    public void notifyDownloadSuccessful(int notificationId, String fileName, Intent intent) {
        NotificationCompat.Builder builder = buildNotification(
                android.R.drawable.stat_sys_download_done,
                fileName,
                mContext.getResources().getString(R.string.download_notification_completed));
        if (intent != null) {
            builder.setContentIntent(PendingIntent.getActivity(
                    mContext, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT));
        }
        updateNotification(notificationId, builder.build());
        removeEntryFromSharedPrefs(notificationId);
    }

    /**
     * Add a download failed notification.
     * @param notificationId Notification ID of the download.
     * @param fileName File name of the download.
     * @param intent Intent to launch when clicking the notification.
     */
    public void notifyDownloadFailed(int notificationId, String fileName) {
        NotificationCompat.Builder builder = buildNotification(
                android.R.drawable.stat_sys_download_done,
                fileName,
                mContext.getResources().getString(R.string.download_notification_failed));
        updateNotification(notificationId, builder.build());
        removeEntryFromSharedPrefs(notificationId);
    }

    /**
     * Called to pause all the download notifications.
     */
    @VisibleForTesting
    void pauseAllDownloads() {
        List<DownloadSharedPreferenceEntry> entries = parseDownloadSharedPrefs(mSharedPrefs);
        for (int i = 0; i < entries.size(); ++i) {
            DownloadSharedPreferenceEntry entry = entries.get(i);
            if (entry.notificationId > 0) {
                notifyDownloadPaused(entry.notificationId, entry.downloadGuid, entry.fileName,
                        entry.isResumable, true);
            }
        }
    }

    private PendingIntent buildPendingIntent(
            String action, int notificationId, String downloadGuid, String fileName) {
        ComponentName component = new ComponentName(
                mContext.getPackageName(), DownloadBroadcastReceiver.class.getName());
        Intent intent = new Intent(action);
        intent.setComponent(component);
        intent.putExtra(EXTRA_DOWNLOAD_NOTIFICATION_ID, notificationId);
        intent.putExtra(EXTRA_DOWNLOAD_GUID, downloadGuid);
        intent.putExtra(EXTRA_DOWNLOAD_FILE_NAME, fileName);
        return PendingIntent.getBroadcast(
                mContext, notificationId, intent, PendingIntent.FLAG_UPDATE_CURRENT);
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
     * Helper method to launch the browser process and handle a download operation that is included
     * in the given intent.
     * @param intent Intent with the download operation.
     */
    private void handleDownloadOperation(final Intent intent) {
        final int notificationId = IntentUtils.safeGetIntExtra(
                intent, DownloadNotificationService.EXTRA_DOWNLOAD_NOTIFICATION_ID, -1);
        final String guid = IntentUtils.safeGetStringExtra(
                intent, DownloadNotificationService.EXTRA_DOWNLOAD_GUID);
        final String fileName = IntentUtils.safeGetStringExtra(
                intent, DownloadNotificationService.EXTRA_DOWNLOAD_FILE_NAME);
        // If browser process already goes away, the download should have already paused. Do nothing
        // in that case.
        if (ACTION_DOWNLOAD_PAUSE.equals(intent.getAction())
                && !DownloadManagerService.hasDownloadManagerService()) {
            return;
        }
        BrowserParts parts = new EmptyBrowserParts() {
            @Override
            public void finishNativeInitialization() {
                DownloadManagerService service =
                        DownloadManagerService.getDownloadManagerService(getApplicationContext());
                switch (intent.getAction()) {
                    case ACTION_DOWNLOAD_CANCEL:
                        // TODO(qinmin): Alternatively, we can delete the downloaded content on
                        // SD card, and remove the download ID from the SharedPreferences so we
                        // don't need to restart the browser process. http://crbug.com/579643.
                        service.cancelDownload(guid);
                        cancelNotification(notificationId);
                        break;
                    case ACTION_DOWNLOAD_PAUSE:
                        service.pauseDownload(guid);
                        break;
                    case ACTION_DOWNLOAD_RESUME:
                        service.resumeDownload(notificationId, guid, fileName, true);
                        notifyDownloadProgress(notificationId, guid, fileName,
                                INVALID_DOWNLOAD_PERCENTAGE, 0, 0, true);
                        break;
                    default:
                        Log.e(TAG, "Unrecognized intent action.", intent);
                        break;
                }
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
     * Checks if an intent requires operations on a download.
     * @param intent An intent to validate.
     * @return true if the intent requires actions, or false otherwise.
     */
    static boolean isDownloadOperationIntent(Intent intent) {
        if (intent == null) return false;
        if (!ACTION_DOWNLOAD_CANCEL.equals(intent.getAction())
                && !ACTION_DOWNLOAD_RESUME.equals(intent.getAction())
                && !ACTION_DOWNLOAD_PAUSE.equals(intent.getAction())) {
            return false;
        }
        if (!intent.hasExtra(EXTRA_DOWNLOAD_NOTIFICATION_ID)
                || !intent.hasExtra(EXTRA_DOWNLOAD_FILE_NAME)
                || !intent.hasExtra(EXTRA_DOWNLOAD_GUID)) {
            return false;
        }
        final int notificationId =
                IntentUtils.safeGetIntExtra(intent, EXTRA_DOWNLOAD_NOTIFICATION_ID, -1);
        if (notificationId == -1) return false;
        final String fileName = IntentUtils.safeGetStringExtra(intent, EXTRA_DOWNLOAD_FILE_NAME);
        if (fileName == null) return false;
        final String guid = IntentUtils.safeGetStringExtra(intent, EXTRA_DOWNLOAD_GUID);
        if (!DownloadSharedPreferenceEntry.isValidGUID(guid)) return false;
        return true;
    }

    /**
     * Add a DownloadSharedPreferenceEntry to SharedPrefs. If the notification ID already exists
     * in SharedPrefs, do nothing.
     * @param DownloadSharedPreferenceEntry A DownloadSharedPreferenceEntry to be added.
     */
    private void addEntryToSharedPrefs(DownloadSharedPreferenceEntry pendingEntry) {
        Set<String> entries = DownloadManagerService.getStoredDownloadInfo(
                mSharedPrefs, PENDING_DOWNLOAD_NOTIFICATIONS);
        for (String entryString : entries) {
            DownloadSharedPreferenceEntry entry =
                    DownloadSharedPreferenceEntry.parseFromString(entryString);
            if (entry.notificationId == pendingEntry.notificationId) return;
        }
        entries.add(pendingEntry.getSharedPreferenceString());
        DownloadManagerService.storeDownloadInfo(
                mSharedPrefs, PENDING_DOWNLOAD_NOTIFICATIONS, entries);
    }

    /**
     * Removes a DownloadSharedPreferenceEntry from SharedPrefs.
     * @param notificationId Notification ID to be removed.
     */
    private void removeEntryFromSharedPrefs(int notificationId) {
        Set<String> entries = DownloadManagerService.getStoredDownloadInfo(
                mSharedPrefs, PENDING_DOWNLOAD_NOTIFICATIONS);
        for (String entryString : entries) {
            DownloadSharedPreferenceEntry entry =
                    DownloadSharedPreferenceEntry.parseFromString(entryString);
            if (entry.notificationId == notificationId) {
                entries.remove(entryString);
                if (entries.isEmpty()) {
                    mSharedPrefs.edit().remove(PENDING_DOWNLOAD_NOTIFICATIONS).apply();
                } else {
                    DownloadManagerService.storeDownloadInfo(
                            mSharedPrefs, PENDING_DOWNLOAD_NOTIFICATIONS, entries);
                }
                break;
            }
        }
    }

    /**
     * Clears all pending downloads from SharedPrefs.
     */
    public void clearPendingDownloads() {
        mSharedPrefs.edit().remove(PENDING_DOWNLOAD_NOTIFICATIONS).apply();
    }

    /**
     * Parse the DownloadSharedPreferenceEntry from the shared preference and return a list of them.
     * @return a list of parsed DownloadSharedPreferenceEntry.
     */
    static List<DownloadSharedPreferenceEntry> parseDownloadSharedPrefs(
            SharedPreferences prefs) {
        List<DownloadSharedPreferenceEntry> result = new ArrayList<DownloadSharedPreferenceEntry>();
        if (prefs.contains(PENDING_DOWNLOAD_NOTIFICATIONS)) {
            Set<String> entries = DownloadManagerService.getStoredDownloadInfo(
                    prefs, PENDING_DOWNLOAD_NOTIFICATIONS);
            for (String entryString : entries) {
                result.add(DownloadSharedPreferenceEntry.parseFromString(entryString));
            }
        }
        return result;
    }
}
