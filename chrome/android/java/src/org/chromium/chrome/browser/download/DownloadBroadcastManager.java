// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static org.chromium.chrome.browser.download.DownloadNotificationService.ACTION_DOWNLOAD_CANCEL;
import static org.chromium.chrome.browser.download.DownloadNotificationService.ACTION_DOWNLOAD_OPEN;
import static org.chromium.chrome.browser.download.DownloadNotificationService.ACTION_DOWNLOAD_PAUSE;
import static org.chromium.chrome.browser.download.DownloadNotificationService.ACTION_DOWNLOAD_RESUME;
import static org.chromium.chrome.browser.download.DownloadNotificationService.ACTION_DOWNLOAD_RESUME_ALL;
import static org.chromium.chrome.browser.download.DownloadNotificationService.getServiceDelegate;

import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.util.Pair;

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
import org.chromium.components.offline_items_collection.ContentId;
import org.chromium.components.offline_items_collection.LegacyHelpers;
import org.chromium.content.browser.BrowserStartupController;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

/**
 * Service that spins up native when an interaction with a notification happens and passes the
 * relevant information on to native.
 */
public final class DownloadBroadcastManager extends Service {
    private static final String TAG = "DLBroadcastManager";
    private static final int WAIT_TIME_MS = 1000;

    private final List<Pair<String, List<DownloadSharedPreferenceEntry>>>
            mNotificationInteractions = new ArrayList<>();
    private final IBinder mBinder = new LocalBinder();

    private boolean mIsNativeInitialized = false;
    private boolean mIsWaitingForNativeInitialized = false;
    private int mCumulativeWaitTimeMs = 0;

    public DownloadBroadcastManager() {}

    public void onNotificationInteraction(
            final Context context, String action, List<DownloadSharedPreferenceEntry> entries) {
        if (!isActionHandled(action)) {
            return;
        }

        mCumulativeWaitTimeMs += WAIT_TIME_MS;

        // On first interaction, initialize native and queue request.
        if (!mIsNativeInitialized && !mIsWaitingForNativeInitialized) {
            mNotificationInteractions.add(new Pair<>(action, entries));
            startServiceInternal(context);
        }

        // If waiting for native initialization, queue requests.
        if (mIsWaitingForNativeInitialized) {
            Preconditions.checkArgument(!mIsNativeInitialized);
            mNotificationInteractions.add(new Pair<>(action, entries));
        }

        // If native is initialized, execute request.
        if (mIsNativeInitialized) {
            Preconditions.checkArgument(!mIsWaitingForNativeInitialized);
            propagateInteraction(action, entries);
        }
    }

    private boolean isActionHandled(String action) {
        return ACTION_DOWNLOAD_CANCEL.equals(action) || ACTION_DOWNLOAD_PAUSE.equals(action)
                || ACTION_DOWNLOAD_RESUME.equals(action)
                || ACTION_DOWNLOAD_RESUME_ALL.equals(action) || ACTION_DOWNLOAD_OPEN.equals(action);
    }

    private boolean isNativeLoaded() {
        return BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                .isStartupSuccessfullyCompleted();
    }

    private void initialInteraction() {
        mIsNativeInitialized = true;
        mIsWaitingForNativeInitialized = false;

        for (Pair<String, List<DownloadSharedPreferenceEntry>> notificationInteraction :
                mNotificationInteractions) {
            propagateInteraction(notificationInteraction.first, notificationInteraction.second);
        }
    }

    private void startServiceInternal(Context context) {
        Intent intent = new Intent();
        intent.setComponent(new ComponentName(context, DownloadBroadcastManager.class));
        context.startService(intent);

        Runnable initialRunnable = new Runnable() {
            @Override
            public void run() {
                initialInteraction();
            }
        };
        runWithNative(context, initialRunnable);
    }

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

    private void stopInternal(int waitTime) {
        new Timer().schedule(new TimerTask() {
            @Override
            public void run() {
                if (mCumulativeWaitTimeMs == 0) {
                    mIsNativeInitialized = false;
                    stopSelf();
                } else {
                    stopInternal(mCumulativeWaitTimeMs);
                    mCumulativeWaitTimeMs = 0;
                }
            }
        }, waitTime);
    }

    private void propagateInteraction(String action, List<DownloadSharedPreferenceEntry> entries) {
        Preconditions.checkArgument(
                ACTION_DOWNLOAD_RESUME_ALL.equals(action) || entries.size() == 1);

        DownloadSharedPreferenceEntry entry =
                ACTION_DOWNLOAD_RESUME_ALL.equals(action) ? null : entries.get(0);
        ContentId id = (entry != null) ? entry.id : null;
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
            case ACTION_DOWNLOAD_RESUME_ALL:
                resumeAllPendingDownloads(entries);
                break;
            case ACTION_DOWNLOAD_OPEN:
                if (LegacyHelpers.isLegacyOfflinePage(id)) {
                    OfflinePageDownloadBridge.openDownloadedPage(id);
                } else if (id != null) {
                    OfflineContentAggregatorNotificationBridgeUiFactory.instance().openItem(id);
                }
                break;
            default:
                // No-op.
                break;
        }

        if (downloadServiceDelegate != null) downloadServiceDelegate.destroyServiceDelegate();
    }

    /**
     * Resumes all pending downloads from SharedPreferences. If a download is
     * already in progress, do nothing.
     */
    public void resumeAllPendingDownloads(List<DownloadSharedPreferenceEntry> entries) {
        if (!DownloadManagerService.hasDownloadManagerService()) return;
        for (int i = 0; i < entries.size(); ++i) {
            DownloadSharedPreferenceEntry entry = entries.get(i);

            DownloadServiceDelegate downloadServiceDelegate = getServiceDelegate(entry.id);
            downloadServiceDelegate.resumeDownload(entry.id, entry.buildDownloadItem(), false);
            downloadServiceDelegate.destroyServiceDelegate();
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        stopInternal(mCumulativeWaitTimeMs);
        mCumulativeWaitTimeMs = 0;
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    /**
     * Class for clients to access.
     */
    private class LocalBinder extends Binder {
        DownloadBroadcastManager getService() {
            return DownloadBroadcastManager.this;
        }
    }
}
