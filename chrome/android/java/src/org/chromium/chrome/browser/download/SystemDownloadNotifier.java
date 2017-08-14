// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.content.Context;

import org.chromium.base.Log;
import org.chromium.components.offline_items_collection.ContentId;

/**
 * DownloadNotifier implementation that creates and updates download notifications.
 * This class creates the {@link DownloadNotificationService} when needed, and binds
 * to the latter to issue calls to show and update notifications.
 */
public class SystemDownloadNotifier implements DownloadNotifier {
    private static final String TAG = "DownloadNotifier";
    //    private static final int DOWNLOAD_NOTIFICATION_TYPE_PROGRESS = 0;
    //    private static final int DOWNLOAD_NOTIFICATION_TYPE_SUCCESS = 1;
    //    private static final int DOWNLOAD_NOTIFICATION_TYPE_FAILURE = 2;
    //    private static final int DOWNLOAD_NOTIFICATION_TYPE_CANCEL = 3;
    //    private static final int DOWNLOAD_NOTIFICATION_TYPE_RESUME_ALL = 4;
    //    private static final int DOWNLOAD_NOTIFICATION_TYPE_PAUSE = 5;
    //    private static final int DOWNLOAD_NOTIFICATION_TYPE_INTERRUPT = 6;
    //    private static final int DOWNLOAD_NOTIFICATION_TYPE_REMOVE_NOTIFICATION = 7;

    private final Context mApplicationContext;
    private final DownloadNotificationService mDownloadNotificationService;
    //    @Nullable private DownloadNotificationService mBoundService;
    //    private Set<String> mActiveDownloads = new HashSet<String>();
    //    private ArrayList<PendingNotificationInfo> mPendingNotifications =
    //            new ArrayList<PendingNotificationInfo>();

    //    private boolean mIsServiceBound;

    /**
     * Pending download notifications to be posted.
     */
    //    static class PendingNotificationInfo {
    //        // Pending download notifications to be posted.
    //        public final int type;
    //        public final DownloadInfo downloadInfo;
    //        public long startTime;
    //        public boolean isAutoResumable;
    //        public boolean canDownloadWhileMetered;
    //        public boolean canResolve;
    //        public long systemDownloadId;
    //        public boolean isSupportedMimeType;
    //        public int notificationId;
    //
    //        public PendingNotificationInfo(int type, DownloadInfo downloadInfo) {
    //            this.type = type;
    //            this.downloadInfo = downloadInfo;
    //        }
    //    }

    /**
     * Constructor.
     * @param context Application context.
     */
    public SystemDownloadNotifier(Context context) {
        mApplicationContext = context.getApplicationContext();
        mDownloadNotificationService = new DownloadNotificationService();
    }

    /**
     * Object to receive information as the service is started and stopped.
     */
    //    private final ServiceConnection mConnection = new ServiceConnection() {
    //        @Override
    //        public void onServiceConnected(ComponentName className, IBinder service) {
    //            if (!(service instanceof DownloadNotificationService.LocalBinder)) {
    //                Log.w(TAG, "Not from DownloadNotificationService, do not connect."
    //                        + " Component name: " + className);
    //                assert false;
    //                return;
    //            }
    //            mBoundService = ((DownloadNotificationService.LocalBinder) service).getService();
    //            mBoundService.addObserver(SystemDownloadNotifier.this);
    //            // updateDownloadNotification() may leave some outstanding notifications
    //            // before the service is connected, handle them now.
    //            handlePendingNotifications();
    //        }
    //
    //        @Override
    //        public void onServiceDisconnected(ComponentName className) {}
    //    };

    /**
     * For tests only: sets the DownloadNotificationService.
     * @param service An instance of DownloadNotificationService.
     */
    //    @VisibleForTesting
    //    void setDownloadNotificationService(DownloadNotificationService service) {
    //        mBoundService = service;
    //    }

    /**
     * Handles all the pending notifications that hasn't been processed.
     */
    //    @VisibleForTesting
    //    void handlePendingNotifications() {
    //        if (mPendingNotifications.isEmpty()) return;
    //        for (int i = 0; i < mPendingNotifications.size(); i++) {
    //            updateDownloadNotification(
    //                    mPendingNotifications.get(i), i == mPendingNotifications.size() - 1);
    //        }
    //        mPendingNotifications.clear();
    //    }

    /**
     * Starts and binds to the download notification service if needed.
     */
    //    private void startAndBindToServiceIfNeeded() {
    //        if (mIsServiceBound) return;
    //        startAndBindService();
    //        mIsServiceBound = true;
    //    }

    /**
     * Stops the download notification service if there are no download in progress.
     */
    //    private void unbindServiceIfNeeded() {
    //        if (!mActiveDownloads.isEmpty() || !mIsServiceBound) return;
    //        if (mBoundService != null) mBoundService.removeObserver(this);
    //        unbindService();
    //        mBoundService = null;
    //        mIsServiceBound = false;
    //    }

    //    @VisibleForTesting
    //    void startAndBindService() {
    //        DownloadNotificationService.startDownloadNotificationService(mApplicationContext,
    //        null); mApplicationContext.bindService(
    //                new Intent(mApplicationContext, DownloadNotificationService.class),
    //                mConnection, Context.BIND_AUTO_CREATE);
    //    }

    //    @VisibleForTesting
    //    void unbindService() {
    //        mApplicationContext.unbindService(mConnection);
    //    }

    //    @Override
    //    public void onDownloadCanceled(ContentId id) {
    //        mActiveDownloads.remove(id.id);
    //        if (mActiveDownloads.isEmpty()) unbindServiceIfNeeded();
    //    }

    @Override
    public void notifyDownloadCanceled(ContentId id) {
        mDownloadNotificationService.notifyDownloadCanceled(id);
    }

    @Override
    public void notifyDownloadSuccessful(DownloadInfo info, long systemDownloadId,
            boolean canResolve, boolean isSupportedMimeType) {
        final int notificationId = mDownloadNotificationService.notifyDownloadSuccessful(
                info.getContentId(), info.getFilePath(), info.getFileName(), systemDownloadId,
                info.isOffTheRecord(), isSupportedMimeType, info.getIsOpenable(), info.getIcon(),
                info.getOriginalUrl(), info.getReferrer());

        if (info.getIsOpenable()) {
            DownloadManagerService.getDownloadManagerService().onSuccessNotificationShown(
                    info, canResolve, notificationId, systemDownloadId);
        }
    }

    @Override
    public void notifyDownloadFailed(DownloadInfo info) {
        mDownloadNotificationService.notifyDownloadFailed(
                info.getContentId(), info.getFileName(), info.getIcon());
    }

    @Override
    public void notifyDownloadProgress(
            DownloadInfo info, long startTime, boolean canDownloadWhileMetered) {
        Log.e("joy", "SDN notifyDownloadProgress " + info.getContentId());
        mDownloadNotificationService.notifyDownloadProgress(info.getContentId(), info.getFileName(),
                info.getProgress(), info.getBytesReceived(), info.getTimeRemainingInMillis(),
                startTime, info.isOffTheRecord(), canDownloadWhileMetered, info.getIsTransient(),
                info.getIcon());
    }

    @Override
    public void notifyDownloadPaused(DownloadInfo info) {
        mDownloadNotificationService.notifyDownloadPaused(info.getContentId(), info.getFileName(),
                true, false, info.isOffTheRecord(), info.getIsTransient(), info.getIcon());
    }

    @Override
    public void notifyDownloadInterrupted(DownloadInfo info, boolean isAutoResumable) {
        mDownloadNotificationService.notifyDownloadPaused(info.getContentId(), info.getFileName(),
                info.isResumable(), isAutoResumable, info.isOffTheRecord(), info.getIsTransient(),
                info.getIcon());
    }

    @Override
    public void removeDownloadNotification(int notificationId, DownloadInfo info) {
        mDownloadNotificationService.cancelNotification(notificationId, info.getContentId());
    }

    @Override
    public void resumePendingDownloads() {
        if (!DownloadNotificationService.isTrackingResumableDownloads(mApplicationContext)) return;
        mDownloadNotificationService.resumeAllPendingDownloads();
    }

    /**
     * Called when a successful notification is shown.
     * @param notificationInfo Pending notification information to be handled.
     * @param notificationId ID of the notification.
     */
    //    @VisibleForTesting
    //    void onSuccessNotificationShown(
    //            final PendingNotificationInfo notificationInfo, final int notificationId) {
    //        if (notificationInfo.downloadInfo == null
    //                || !notificationInfo.downloadInfo.getIsOpenable()) {
    //            return;
    //        }
    //
    //    }

    /**
     * Helper method to schedule download notification updates.
     * @param notificationInfo Pending notification information to be handled.
     * @param autoRelease Whether or not to allow unbinding the service after processing the action.
     */
    //    @VisibleForTesting
    //    void updateDownloadNotification(
    //            final PendingNotificationInfo notificationInfo, boolean autoRelease) {
    //        assert ThreadUtils.runningOnUiThread();
    ////        startAndBindToServiceIfNeeded();
    //
    ////        if (mBoundService == null) {
    ////            mPendingNotifications.add(notificationInfo);
    ////            return;
    ////        }
    //
    //        DownloadInfo info = notificationInfo.downloadInfo;
    ////        if (notificationInfo.type == DOWNLOAD_NOTIFICATION_TYPE_PROGRESS) {
    ////            mActiveDownloads.add(info.getDownloadGuid());
    ////        } else if (notificationInfo.type != DOWNLOAD_NOTIFICATION_TYPE_RESUME_ALL) {
    ////            mActiveDownloads.remove(info.getDownloadGuid());
    ////        }
    //
    //        switch (notificationInfo.type) {
    //            case DOWNLOAD_NOTIFICATION_TYPE_PROGRESS:
    //                mDownloadNotificationService.notifyDownloadProgress(info.getContentId(),
    //                        info.getFileName(), info.getProgress(), info.getBytesReceived(),
    //                        info.getTimeRemainingInMillis(), notificationInfo.startTime,
    //                        info.isOffTheRecord(), notificationInfo.canDownloadWhileMetered,
    //                        info.getIsTransient(), info.getIcon());
    //                break;
    //            case DOWNLOAD_NOTIFICATION_TYPE_PAUSE:
    //
    //                break;
    //            case DOWNLOAD_NOTIFICATION_TYPE_INTERRUPT:
    //
    //                break;
    //            case DOWNLOAD_NOTIFICATION_TYPE_SUCCESS:
    //
    //                break;
    //            case DOWNLOAD_NOTIFICATION_TYPE_FAILURE:
    //
    //                break;
    //            case DOWNLOAD_NOTIFICATION_TYPE_CANCEL:
    //
    //                break;
    //            case DOWNLOAD_NOTIFICATION_TYPE_RESUME_ALL:
    //                mDownloadNotificationService.resumeAllPendingDownloads();
    //                break;
    //            case DOWNLOAD_NOTIFICATION_TYPE_REMOVE_NOTIFICATION:
    //
    //                break;
    //            default:
    //                assert false;
    //        }
    //
    //        // Don't need to expose the notification id to ignore.  Cancel will automatically call
    //        this
    //        // method as well and pass it in.
    ////        if (mBoundService != null) mBoundService.hideSummaryNotificationIfNecessary(-1);
    ////        if (autoRelease) unbindServiceIfNeeded();
    //    }
}
