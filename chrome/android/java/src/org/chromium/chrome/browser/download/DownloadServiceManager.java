// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.app.Notification;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.support.annotation.Nullable;

import org.chromium.base.VisibleForTesting;

import java.util.ArrayList;
import java.util.List;

/**
 * Manager to stop and start the foreground service associated with downloads.
 */
public class DownloadServiceManager extends Service {
    private final IBinder mBinder = new LocalBinder();
    private final List<Integer> mActiveNotifications = new ArrayList<>();

    private int mPinnedNotification = -1;

    /**
     * Called by DownloadNotificationStore when there is a change in the list of downloads.
     * @param isActive whether the download is "active" (started/resumed) or not.
     * @param notificationId identifier of the notification associated with the download.
     * @param notification the actual notification associated with the download.
     */
    public void updateDownloadStatus(
            boolean isActive, Integer notificationId, Notification notification) {
        if (isActive) {
            mActiveNotifications.add(notificationId);
            if (mPinnedNotification < 0) {
                mPinnedNotification = notificationId;
                startForegroundInternal(notificationId, notification);
            }
        } else {
            if (mActiveNotifications.contains(notificationId)) {
                mActiveNotifications.remove(notificationId);
                if (mActiveNotifications.isEmpty()) {
                    mPinnedNotification = -1;
                    stopForegroundInternal();
                    return;
                }
                if (mPinnedNotification == notificationId) {
                    mPinnedNotification = mActiveNotifications.get(0);
                }
            }
        }
    }

    @VisibleForTesting
    void startForegroundInternal(int notificationId, Notification notification) {
        startForeground(notificationId, notification);
    }

    @VisibleForTesting
    void stopForegroundInternal() {
        stopForeground(true /* killNotification */);
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
        DownloadServiceManager getService() {
            return DownloadServiceManager.this;
        }
    }
}
