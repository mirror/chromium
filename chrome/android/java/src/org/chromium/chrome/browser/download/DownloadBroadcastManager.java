// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static org.chromium.chrome.browser.download.DownloadNotificationService.ACTION_DOWNLOAD_CANCEL;
import static org.chromium.chrome.browser.download.DownloadNotificationService.ACTION_DOWNLOAD_MISSING_NOTIFICATION;
import static org.chromium.chrome.browser.download.DownloadNotificationService.ACTION_DOWNLOAD_OPEN;
import static org.chromium.chrome.browser.download.DownloadNotificationService.ACTION_DOWNLOAD_PAUSE;
import static org.chromium.chrome.browser.download.DownloadNotificationService.ACTION_DOWNLOAD_RESUME;
import static org.chromium.chrome.browser.download.DownloadNotificationService.ACTION_DOWNLOAD_RESUME_ALL;
import static org.chromium.chrome.browser.download.DownloadNotificationService.EXTRA_IS_OFF_THE_RECORD;
import static org.chromium.chrome.browser.download.DownloadNotificationService.getContentIdFromIntent;
import static org.chromium.chrome.browser.download.DownloadNotificationService.getServiceDelegate;

import android.content.Context;
import android.content.Intent;
import android.text.TextUtils;

import com.google.ipc.invalidation.util.Preconditions;

import org.chromium.base.Log;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.chrome.browser.ChromeApplication;
import org.chromium.chrome.browser.download.items.OfflineContentAggregatorNotificationBridgeUiFactory;
import org.chromium.chrome.browser.init.BrowserParts;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.init.EmptyBrowserParts;
import org.chromium.chrome.browser.offlinepages.downloads.OfflinePageDownloadBridge;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.components.offline_items_collection.ContentId;
import org.chromium.components.offline_items_collection.LegacyHelpers;
import org.chromium.content.browser.BrowserStartupController;

import java.util.Timer;
import java.util.TimerTask;

/**
 * Class that spins up native when an interaction with a notification happens and passes the
 * relevant information on to native.
 */
public final class DownloadBroadcastManager {
    private static final String TAG = "DLBroadcastManager";
    private static final int WAIT_TIME_MS = 1000;

    private final DownloadSharedPreferenceHelper mDownloadSharedPreferenceHelper =
            DownloadSharedPreferenceHelper.getInstance();

    private int mCumulativeWaitTimeMs = 0;

    public DownloadBroadcastManager() {}

    /**
     * Passes down information about a notification interaction to native.
     * TODO(jming): Move this call to onStartCommand when this class becomes a service.
     * @param context of the interaction.
     * @param intent with information about the notification interaction (action, contentId, etc).
     */
    public void onNotificationInteraction(Context context, final Intent intent) {
        String action = intent.getAction();
        if (!isActionHandled(action)) {
            return;
        }

        mCumulativeWaitTimeMs += WAIT_TIME_MS;

        Runnable initialRunnable = new Runnable() {
            @Override
            public void run() {
                propagateInteraction(intent);
            }
        };
        runWithNative(context, initialRunnable);

        stopInternal(mCumulativeWaitTimeMs);
        mCumulativeWaitTimeMs = 0;
    }

    private void propagateInteraction(Intent intent) {
        String action = intent.getAction();
        final ContentId id = getContentIdFromIntent(intent);
        final DownloadSharedPreferenceEntry entry = getDownloadEntryFromIntent(intent);
        DownloadServiceDelegate downloadServiceDelegate =
                ACTION_DOWNLOAD_OPEN.equals(action) ? null : getServiceDelegate(id);

        switch (action) {
            case ACTION_DOWNLOAD_CANCEL:
                Preconditions.checkNotNull(downloadServiceDelegate);
                Preconditions.checkNotNull(entry);

                downloadServiceDelegate.cancelDownload(entry.id, entry.isOffTheRecord);
                break;

            case ACTION_DOWNLOAD_PAUSE:
                Preconditions.checkNotNull(downloadServiceDelegate);
                Preconditions.checkNotNull(entry);

                downloadServiceDelegate.pauseDownload(entry.id, entry.isOffTheRecord);
                break;

            case ACTION_DOWNLOAD_RESUME:
                Preconditions.checkNotNull(downloadServiceDelegate);
                Preconditions.checkNotNull(entry);

                downloadServiceDelegate.resumeDownload(
                        entry.id, entry.buildDownloadItem(), true /* hasUserGesture */);
                break;

            case ACTION_DOWNLOAD_OPEN:
                if (LegacyHelpers.isLegacyOfflinePage(id)) {
                    OfflinePageDownloadBridge.openDownloadedPage(id);
                } else if (id != null) {
                    OfflineContentAggregatorNotificationBridgeUiFactory.instance().openItem(id);
                }
                break;

            case ACTION_DOWNLOAD_MISSING_NOTIFICATION:
                // Handles operations for downloads DownloadNotificationService is unaware of.
                // This can happen because the DownloadNotificationService learns about downloads
                // later than Download Home does, and may not have a DownloadSharedPreferenceEntry.
                // TODO(qinmin): Fix the SharedPreferences so that it properly tracks entries.

                // Should only be called via Download Home, but catch this case to be safe.
                if (!DownloadManagerService.hasDownloadManagerService()) return;

                boolean isOffTheRecord =
                        IntentUtils.safeGetBooleanExtra(intent, EXTRA_IS_OFF_THE_RECORD, false);
                if (!LegacyHelpers.isLegacyDownload(id)) return;

                // Pass information directly to the DownloadManagerService.
                if (TextUtils.equals(action, ACTION_DOWNLOAD_CANCEL)) {
                    getServiceDelegate(id).cancelDownload(id, isOffTheRecord);
                } else if (TextUtils.equals(action, ACTION_DOWNLOAD_PAUSE)) {
                    getServiceDelegate(id).pauseDownload(id, isOffTheRecord);
                } else if (TextUtils.equals(action, ACTION_DOWNLOAD_RESUME)) {
                    DownloadInfo info = new DownloadInfo.Builder()
                                                .setDownloadGuid(id.id)
                                                .setIsOffTheRecord(isOffTheRecord)
                                                .build();
                    getServiceDelegate(id).resumeDownload(id, new DownloadItem(false, info), true);
                }
                break;

            default:
                // No-op.
                break;
        }

        if (downloadServiceDelegate != null) downloadServiceDelegate.destroyServiceDelegate();
    }

    /**
     * Helper function that loads the native and runs given runnable.
     * @param context of the application.
     * @param startWithNativeRunnable that is run when the native is loaded.
     */
    private void runWithNative(Context context, final Runnable startWithNativeRunnable) {
        if (isNativeLoaded()) {
            startWithNativeRunnable.run();
            return;
        }

        final BrowserParts parts = new EmptyBrowserParts() {
            @Override
            public void finishNativeInitialization() {
                // Make sure the OfflineContentAggregator bridge is initialized.
                OfflineContentAggregatorNotificationBridgeUiFactory.instance();
                startWithNativeRunnable.run();
            }
        };

        try {
            ChromeBrowserInitializer.getInstance(context).handlePreNativeStartup(parts);
            ChromeBrowserInitializer.getInstance(context).handlePostNativeStartup(true, parts);
        } catch (ProcessInitException e) {
            Log.e(TAG, "Unable to load native library.", e);
            ChromeApplication.reportStartupErrorAndExit(e);
        }
    }

    /**
     * Waits and checks to see if more wait time has been accumulated or stops the service.
     * @param waitTimeMs the given wait time in milliseconds.
     */
    private void stopInternal(int waitTimeMs) {
        new Timer().schedule(new TimerTask() {
            @Override
            public void run() {
                if (mCumulativeWaitTimeMs == 0) {
                    // TODO(jming): Add the "stopSelf()" call when this becomes a service.
                } else {
                    stopInternal(mCumulativeWaitTimeMs);
                    mCumulativeWaitTimeMs = 0;
                }
            }
        }, waitTimeMs);
    }

    private boolean isActionHandled(String action) {
        return ACTION_DOWNLOAD_CANCEL.equals(action) || ACTION_DOWNLOAD_PAUSE.equals(action)
                || ACTION_DOWNLOAD_RESUME.equals(action) || ACTION_DOWNLOAD_OPEN.equals(action);
    }

    private boolean isNativeLoaded() {
        return BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                .isStartupSuccessfullyCompleted();
    }

    /**
     * Retrieves DownloadSharedPreferenceEntry from a download action intent.
     * TODO(jming): Make private when no longer needed by DownloadNotificationService.
     * @param intent Intent that contains the download action.
     */
    DownloadSharedPreferenceEntry getDownloadEntryFromIntent(Intent intent) {
        if (ACTION_DOWNLOAD_RESUME_ALL.equals(intent.getAction())) return null;
        return mDownloadSharedPreferenceHelper.getDownloadSharedPreferenceEntry(
                getContentIdFromIntent(intent));
    }
}
