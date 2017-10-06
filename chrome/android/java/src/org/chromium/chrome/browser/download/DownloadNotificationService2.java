// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static org.chromium.chrome.browser.download.DownloadBroadcastManager.getServiceDelegate;

import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.shapes.OvalShape;
import android.text.TextUtils;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.notifications.NotificationUmaTracker;
import org.chromium.chrome.browser.notifications.channels.ChannelDefinitions;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.offline_items_collection.ContentId;
import org.chromium.components.offline_items_collection.LegacyHelpers;
import org.chromium.components.offline_items_collection.OfflineItem;
import org.chromium.components.offline_items_collection.OfflineItem.Progress;
import org.chromium.content.browser.BrowserStartupController;

import java.util.ArrayList;
import java.util.List;

/**
 * Central director for updates related to downloads and notifications.
 *  - Receive updates about downloads through SystemDownloadNotifier (notifyDownloadPaused, etc).
 *  - Create notifications for downloads using DownloadNotificationFactory.
 *  - Update DownloadForegroundServiceManager about downloads, allowing it to start/stop service.
 */
public class DownloadNotificationService2 {
    static final String EXTRA_DOWNLOAD_CONTENTID_ID =
            "org.chromium.chrome.browser.download.DownloadContentId_Id";
    static final String EXTRA_DOWNLOAD_CONTENTID_NAMESPACE =
            "org.chromium.chrome.browser.download.DownloadContentId_Namespace";
    static final String EXTRA_DOWNLOAD_FILE_PATH = "DownloadFilePath";
    static final String EXTRA_IS_SUPPORTED_MIME_TYPE = "IsSupportedMimeType";
    static final String EXTRA_IS_OFF_THE_RECORD =
            "org.chromium.chrome.browser.download.IS_OFF_THE_RECORD";

    public static final String ACTION_DOWNLOAD_CANCEL =
            "org.chromium.chrome.browser.download.DOWNLOAD_CANCEL";
    public static final String ACTION_DOWNLOAD_PAUSE =
            "org.chromium.chrome.browser.download.DOWNLOAD_PAUSE";
    public static final String ACTION_DOWNLOAD_RESUME =
            "org.chromium.chrome.browser.download.DOWNLOAD_RESUME";
    public static final String ACTION_DOWNLOAD_OPEN =
            "org.chromium.chrome.browser.download.DOWNLOAD_OPEN";

    public static final String EXTRA_NOTIFICATION_BUNDLE_ICON_ID =
            "Chrome.NotificationBundleIconIdExtra";
    /** Notification Id starting value, to avoid conflicts from IDs used in prior versions. */
    private static final int STARTING_NOTIFICATION_ID = 1000000;

    private static final String KEY_NEXT_DOWNLOAD_NOTIFICATION_ID = "NextDownloadNotificationId";

    private static final int MAX_RESUMPTION_ATTEMPT_LEFT = 5;
    private static final String KEY_AUTO_RESUMPTION_ATTEMPT_LEFT = "ResumptionAttemptLeft";

    @VisibleForTesting
    final List<ContentId> mDownloadsInProgress = new ArrayList<ContentId>();

    private NotificationManager mNotificationManager;
    private SharedPreferences mSharedPrefs;
    private int mNextNotificationId;
    private Bitmap mDownloadSuccessLargeIcon;
    private DownloadSharedPreferenceHelper mDownloadSharedPreferenceHelper;
    private DownloadForegroundServiceManager mDownloadForegroundServiceManager;

    private static class LazyHolder {
        private static final DownloadNotificationService2 INSTANCE =
                new DownloadNotificationService2();
    }

    /**
     * Creates DownloadNotificationService.
     */
    public static DownloadNotificationService2 getInstance() {
        return LazyHolder.INSTANCE;
    }

    @VisibleForTesting
    DownloadNotificationService2() {
        mNotificationManager =
                (NotificationManager) ContextUtils.getApplicationContext().getSystemService(
                        Context.NOTIFICATION_SERVICE);
        mSharedPrefs = ContextUtils.getAppSharedPreferences();
        mDownloadSharedPreferenceHelper = DownloadSharedPreferenceHelper.getInstance();
        mNextNotificationId =
                mSharedPrefs.getInt(KEY_NEXT_DOWNLOAD_NOTIFICATION_ID, STARTING_NOTIFICATION_ID);
        mDownloadForegroundServiceManager = new DownloadForegroundServiceManager();
    }

    @VisibleForTesting
    void setDownloadForegroundServiceManager(
            DownloadForegroundServiceManager downloadForegroundServiceManager) {
        mDownloadForegroundServiceManager = downloadForegroundServiceManager;
    }

    /**
     * @return Whether or not there are any current resumable downloads being tracked.  These
     *         tracked downloads may not currently be showing notifications.
     */
    static boolean isTrackingResumableDownloads(Context context) {
        List<DownloadSharedPreferenceEntry> entries =
                DownloadSharedPreferenceHelper.getInstance().getEntries();
        for (DownloadSharedPreferenceEntry entry : entries) {
            if (canResumeDownload(context, entry)) return true;
        }
        return false;
    }

    /**
     * Track in-progress downloads here.
     * @param id The {@link ContentId} of the download that has been started and should be tracked.
     */
    private void startTrackingInProgressDownload(ContentId id) {
        if (!mDownloadsInProgress.contains(id)) mDownloadsInProgress.add(id);
    }

    /**
     * Stop tracking the download represented by {@code id}.
     * @param id                  The {@link ContentId} of the download that has been paused or
     *                            canceled and shouldn't be tracked.
     */
    private void stopTrackingInProgressDownload(ContentId id) {
        mDownloadsInProgress.remove(id);
    }

    /**
     * Adds or updates an in-progress download notification.
     * @param offlineItem               Information about the download.
     */
    @VisibleForTesting
    public void notifyDownloadProgress(OfflineItem offlineItem) {
        offlineItem.isPending = false;
        updateActiveDownloadNotification(offlineItem, false);
    }

    /**
     * Adds or updates a pending download notification.
     * @param offlineItem               Information about the download.
     * @param hasUserGesture            Whether this request to update the notification comes from
     *                                  a user action (DownloadBroadcastManager) or native update.
     */
    void notifyDownloadPending(OfflineItem offlineItem, boolean hasUserGesture) {
        offlineItem.progress = Progress.createIndeterminateProgress();
        offlineItem.timeRemainingMs = 0;
        offlineItem.creationTimeMs = 0;
        offlineItem.isPending = true;

        updateActiveDownloadNotification(offlineItem, hasUserGesture);
    }

    /**
     * Helper method to update the notification for an active download, the download is either in
     * progress or pending.
     * @param offlineItem               Information about the download.
     * @param hasUserGesture            Whether this request to update the notification comes from
     *                                  a user action (DownloadBroadcastManager) or native update.
     */
    private void updateActiveDownloadNotification(OfflineItem offlineItem, boolean hasUserGesture) {
        offlineItem.notificationId = getNotificationId(offlineItem.id);
        Context context = ContextUtils.getApplicationContext();

        Notification notification = DownloadNotificationFactory.buildNotification(
                context, DownloadNotificationFactory.DownloadStatus.IN_PROGRESS, offlineItem);

        // If called from DownloadBroadcastManager, only update notification, not tracking.
        if (hasUserGesture) {
            updateNotification(offlineItem.notificationId, notification);
            return;
        }

        offlineItem.isAutoResumable = true;
        updateNotification(offlineItem.notificationId, notification, offlineItem.id,
                new DownloadSharedPreferenceEntry(offlineItem));
        // TODO(jming): do we want to handle the pending option in a different manner?
        mDownloadForegroundServiceManager.updateDownloadStatus(context,
                DownloadForegroundServiceManager.DownloadStatus.IN_PROGRESS,
                offlineItem.notificationId, notification);

        startTrackingInProgressDownload(offlineItem.id);
    }

    public void cancelNotification(int notificationId) {
        // TODO(b/65052774): Add back NOTIFICATION_NAMESPACE when able to.
        mNotificationManager.cancel(notificationId);
    }

    /**
     * Removes a download notification and all associated tracking.  This method relies on the
     * caller to provide the notification id, which is useful in the case where the internal
     * tracking doesn't exist (e.g. in the case of a successful download, where we show the download
     * completed notification and remove our internal state tracking).
     * @param notificationId Notification ID of the download
     * @param id The {@link ContentId} of the download.
     */
    public void cancelNotification(int notificationId, ContentId id) {
        cancelNotification(notificationId);
        mDownloadSharedPreferenceHelper.removeSharedPreferenceEntry(id);

        stopTrackingInProgressDownload(id);
    }

    /**
     * Called when a download is canceled.  This method uses internal tracking to try to find the
     * notification id to cancel.
     * @param id The {@link ContentId} of the download.
     */
    @VisibleForTesting
    public void notifyDownloadCanceled(ContentId id, boolean hasUserGesture) {
        DownloadSharedPreferenceEntry entry =
                mDownloadSharedPreferenceHelper.getDownloadSharedPreferenceEntry(id);
        if (entry == null) return;

        // If called from DownloadBroadcastManager, only update notification, not tracking.
        if (hasUserGesture) {
            cancelNotification(entry.notificationId);
            return;
        }

        cancelNotification(entry.notificationId, id);
        mDownloadForegroundServiceManager.updateDownloadStatus(ContextUtils.getApplicationContext(),
                DownloadForegroundServiceManager.DownloadStatus.CANCEL, entry.notificationId, null);
    }

    /**
     * Change a download notification to paused state.
     * @param offlineItem               Information about the download.
     * @param hasUserGesture            Whether this request to update the notification comes from
     *                                  a user action (DownloadBroadcastManager) or native update.
     */
    @VisibleForTesting
    void notifyDownloadPaused(OfflineItem offlineItem, boolean hasUserGesture) {
        DownloadSharedPreferenceEntry entry =
                mDownloadSharedPreferenceHelper.getDownloadSharedPreferenceEntry(offlineItem.id);
        if (!offlineItem.isResumable) {
            notifyDownloadFailed(offlineItem);
            return;
        }
        // If download is already paused, do nothing.
        if (entry != null && !entry.isAutoResumable) {
            return;
        }
        boolean canDownloadWhileMetered = entry != null && entry.canDownloadWhileMetered;
        // If download is interrupted due to network disconnection, show download pending state.
        if (offlineItem.isAutoResumable) {
            offlineItem.allowMetered = canDownloadWhileMetered;
            notifyDownloadPending(offlineItem, hasUserGesture);
            stopTrackingInProgressDownload(offlineItem.id);
            return;
        }

        offlineItem.notificationId =
                entry == null ? getNotificationId(offlineItem.id) : entry.notificationId;
        Context context = ContextUtils.getApplicationContext();

        Notification notification = DownloadNotificationFactory.buildNotification(
                context, DownloadNotificationFactory.DownloadStatus.PAUSED, offlineItem);

        // If called from DownloadBroadcastManager, only update notification, not tracking.
        if (hasUserGesture) {
            updateNotification(offlineItem.notificationId, notification);
            return;
        }

        updateNotification(offlineItem.notificationId, notification, offlineItem.id,
                new DownloadSharedPreferenceEntry(offlineItem));

        mDownloadForegroundServiceManager.updateDownloadStatus(context,
                DownloadForegroundServiceManager.DownloadStatus.PAUSE, offlineItem.notificationId,
                notification);

        stopTrackingInProgressDownload(offlineItem.id);
    }

    /**
     * Add a download successful notification.
     * @param offlineItem               Information about the download.
     * @return                    ID of the successful download notification. Used for removing the
     *                            notification when user click on the snackbar.
     */
    @VisibleForTesting
    public int notifyDownloadSuccessful(OfflineItem offlineItem) {
        Context context = ContextUtils.getApplicationContext();
        offlineItem.notificationId = getNotificationId(offlineItem.id);

        if (offlineItem.icon == null && mDownloadSuccessLargeIcon == null) {
            Bitmap bitmap =
                    BitmapFactory.decodeResource(context.getResources(), R.drawable.offline_pin);
            mDownloadSuccessLargeIcon = getLargeNotificationIcon(bitmap);
        }
        if (offlineItem.icon == null) offlineItem.icon = mDownloadSuccessLargeIcon;

        Notification notification = DownloadNotificationFactory.buildNotification(
                context, DownloadNotificationFactory.DownloadStatus.SUCCESSFUL, offlineItem);

        updateNotification(offlineItem.notificationId, notification, offlineItem.id, null);
        mDownloadForegroundServiceManager.updateDownloadStatus(context,
                DownloadForegroundServiceManager.DownloadStatus.COMPLETE,
                offlineItem.notificationId, notification);
        stopTrackingInProgressDownload(offlineItem.id);
        return offlineItem.notificationId;
    }

    /**
     * Add a download failed notification.
     * @param offlineItem               Information about the download.
     */
    @VisibleForTesting
    public void notifyDownloadFailed(OfflineItem offlineItem) {
        // If the download is not in history db, fileName could be empty. Get it from
        // SharedPreferences.
        if (TextUtils.isEmpty(offlineItem.fileName)) {
            DownloadSharedPreferenceEntry entry =
                    mDownloadSharedPreferenceHelper.getDownloadSharedPreferenceEntry(
                            offlineItem.id);
            if (entry == null) return;
            offlineItem.fileName = entry.fileName;
        }

        offlineItem.notificationId = getNotificationId(offlineItem.id);
        Context context = ContextUtils.getApplicationContext();

        Notification notification = DownloadNotificationFactory.buildNotification(
                context, DownloadNotificationFactory.DownloadStatus.FAILED, offlineItem);

        updateNotification(offlineItem.notificationId, notification, offlineItem.id, null);
        mDownloadForegroundServiceManager.updateDownloadStatus(context,
                DownloadForegroundServiceManager.DownloadStatus.FAIL, offlineItem.notificationId,
                notification);

        stopTrackingInProgressDownload(offlineItem.id);
    }

    private Bitmap getLargeNotificationIcon(Bitmap bitmap) {
        Resources resources = ContextUtils.getApplicationContext().getResources();
        int height = (int) resources.getDimension(android.R.dimen.notification_large_icon_height);
        int width = (int) resources.getDimension(android.R.dimen.notification_large_icon_width);
        final OvalShape circle = new OvalShape();
        circle.resize(width, height);
        final Paint paint = new Paint();
        paint.setColor(ApiCompatibilityUtils.getColor(resources, R.color.google_blue_grey_500));

        final Bitmap result = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(result);
        circle.draw(canvas, paint);
        float leftOffset = (width - bitmap.getWidth()) / 2f;
        float topOffset = (height - bitmap.getHeight()) / 2f;
        if (leftOffset >= 0 && topOffset >= 0) {
            canvas.drawBitmap(bitmap, leftOffset, topOffset, null);
        } else {
            // Scale down the icon into the notification icon dimensions
            canvas.drawBitmap(bitmap, new Rect(0, 0, bitmap.getWidth(), bitmap.getHeight()),
                    new Rect(0, 0, width, height), null);
        }
        return result;
    }

    @VisibleForTesting
    void updateNotification(int id, Notification notification) {
        // TODO(b/65052774): Add back NOTIFICATION_NAMESPACE when able to.
        mNotificationManager.notify(id, notification);
    }

    private void updateNotification(int notificationId, Notification notification, ContentId id,
            DownloadSharedPreferenceEntry entry) {
        updateNotification(notificationId, notification);
        trackNotificationUma(id);

        if (entry != null) {
            mDownloadSharedPreferenceHelper.addOrReplaceSharedPreferenceEntry(entry);
        } else {
            mDownloadSharedPreferenceHelper.removeSharedPreferenceEntry(id);
        }
    }

    private void trackNotificationUma(ContentId id) {
        // Check if we already have an entry in the DownloadSharedPreferenceHelper.  This is a
        // reasonable indicator for whether or not a notification is already showing (or at least if
        // we had built one for this download before.
        if (mDownloadSharedPreferenceHelper.hasEntry(id)) return;
        NotificationUmaTracker.getInstance().onNotificationShown(
                LegacyHelpers.isLegacyOfflinePage(id) ? NotificationUmaTracker.DOWNLOAD_PAGES
                                                      : NotificationUmaTracker.DOWNLOAD_FILES,
                ChannelDefinitions.CHANNEL_ID_DOWNLOADS);
    }

    private static boolean canResumeDownload(Context context, DownloadSharedPreferenceEntry entry) {
        if (entry == null) return false;
        if (!entry.isAutoResumable) return false;

        boolean isNetworkMetered = DownloadManagerService.isActiveNetworkMetered(context);
        return entry.canDownloadWhileMetered || !isNetworkMetered;
    }

    /**
     * Resumes all pending downloads from SharedPreferences. If a download is
     * already in progress, do nothing.
     */
    void resumeAllPendingDownloads() {
        Context context = ContextUtils.getApplicationContext();

        // Limit the number of auto resumption attempts in case Chrome falls into a vicious cycle.
        DownloadResumptionScheduler.getDownloadResumptionScheduler(context).cancelTask();
        int numAutoResumptionAtemptLeft = getResumptionAttemptLeft();
        if (numAutoResumptionAtemptLeft <= 0) return;

        numAutoResumptionAtemptLeft--;
        updateResumptionAttemptLeft(numAutoResumptionAtemptLeft);

        // Go through and check which downloads to resume.
        List<DownloadSharedPreferenceEntry> entries = mDownloadSharedPreferenceHelper.getEntries();
        for (int i = 0; i < entries.size(); ++i) {
            DownloadSharedPreferenceEntry entry = entries.get(i);
            if (!canResumeDownload(context, entry)) continue;
            if (mDownloadsInProgress.contains(entry.id)) continue;

            OfflineItem offlineItem = DownloadSharedPreferenceEntry.offlineItem(entry);
            offlineItem.allowMetered = entry.canDownloadWhileMetered;
            offlineItem.icon = null;

            notifyDownloadPending(offlineItem, false);

            Intent intent = new Intent();
            intent.setAction(ACTION_DOWNLOAD_RESUME);
            intent.putExtra(EXTRA_DOWNLOAD_CONTENTID_ID, entry.id.id);
            intent.putExtra(EXTRA_DOWNLOAD_CONTENTID_NAMESPACE, entry.id.namespace);

            resumeDownload(intent);
        }
    }

    @VisibleForTesting
    void resumeDownload(Intent intent) {
        DownloadBroadcastManager.startDownloadBroadcastManager(
                ContextUtils.getApplicationContext(), intent);
    }

    /**
     * Return the notification ID for the given download {@link ContentId}.
     * @param id the {@link ContentId} of the download.
     * @return notification ID to be used.
     */
    private int getNotificationId(ContentId id) {
        DownloadSharedPreferenceEntry entry =
                mDownloadSharedPreferenceHelper.getDownloadSharedPreferenceEntry(id);
        if (entry != null) return entry.notificationId;
        int notificationId = mNextNotificationId;
        mNextNotificationId = mNextNotificationId == Integer.MAX_VALUE ? STARTING_NOTIFICATION_ID
                                                                       : mNextNotificationId + 1;

        SharedPreferences.Editor editor = mSharedPrefs.edit();
        editor.putInt(KEY_NEXT_DOWNLOAD_NOTIFICATION_ID, mNextNotificationId);
        editor.apply();
        return notificationId;
    }

    /**
     * Helper method to update the remaining number of background resumption attempts left.
     */
    private static void updateResumptionAttemptLeft(int numAutoResumptionAttemptLeft) {
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putInt(KEY_AUTO_RESUMPTION_ATTEMPT_LEFT, numAutoResumptionAttemptLeft)
                .apply();
    }

    /**
     * Helper method to get the remaining number of background resumption attempts left.
     */
    private static int getResumptionAttemptLeft() {
        SharedPreferences sharedPrefs = ContextUtils.getAppSharedPreferences();
        return sharedPrefs.getInt(KEY_AUTO_RESUMPTION_ATTEMPT_LEFT, MAX_RESUMPTION_ATTEMPT_LEFT);
    }

    /**
     * Helper method to clear the remaining number of background resumption attempts left.
     */
    static void clearResumptionAttemptLeft() {
        ContextUtils.getAppSharedPreferences()
                .edit()
                .remove(KEY_AUTO_RESUMPTION_ATTEMPT_LEFT)
                .apply();
    }

    void onForegroundServiceRestarted() {
        updateNotificationsForShutdown();
        resumeAllPendingDownloads();
    }

    void onForegroundServiceTaskRemoved() {
        // If we've lost all Activities, cancel the off the record downloads.
        if (ApplicationStatus.isEveryActivityDestroyed()) {
            cancelOffTheRecordDownloads();
        }
    }

    void onForegroundServiceDestroyed() {
        updateNotificationsForShutdown();
        rescheduleDownloads();
    }

    private void updateNotificationsForShutdown() {
        cancelOffTheRecordDownloads();
        List<DownloadSharedPreferenceEntry> entries = mDownloadSharedPreferenceHelper.getEntries();
        for (DownloadSharedPreferenceEntry entry : entries) {
            if (entry.isOffTheRecord) continue;
            // Move all regular downloads to pending.  Don't propagate the pause because
            // if native is still working and it triggers an update, then the service will be
            // restarted.
            OfflineItem offlineItem = DownloadSharedPreferenceEntry.offlineItem(entry);
            offlineItem.isResumable = true;
            offlineItem.isAutoResumable = true;
            offlineItem.icon = null;

            notifyDownloadPaused(offlineItem, false);
        }
    }

    private void cancelOffTheRecordDownloads() {
        boolean cancelActualDownload =
                BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                        .isStartupSuccessfullyCompleted()
                && Profile.getLastUsedProfile().hasOffTheRecordProfile();

        List<DownloadSharedPreferenceEntry> entries = mDownloadSharedPreferenceHelper.getEntries();
        List<DownloadSharedPreferenceEntry> copies =
                new ArrayList<DownloadSharedPreferenceEntry>(entries);
        for (DownloadSharedPreferenceEntry entry : copies) {
            if (!entry.isOffTheRecord) continue;
            ContentId id = entry.id;
            notifyDownloadCanceled(id, false);
            if (cancelActualDownload) {
                DownloadServiceDelegate delegate = getServiceDelegate(id);
                delegate.cancelDownload(id, true);
                delegate.destroyServiceDelegate();
            }
        }
    }

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
}
