// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.app.Notification;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.support.annotation.Nullable;

import org.chromium.base.VisibleForTesting;

import java.util.HashMap;
import java.util.Map;

/**
 * Manager to stop and start the foreground service associated with downloads.
 */
public class DownloadServiceManager extends Service {
    // To get around b/36922517.
    @VisibleForTesting
    boolean mIsJUnitTest = false;

    private final IBinder mBinder = new LocalBinder();
    private final Map<Integer, Notification> mActiveNotifications = new HashMap<>();

    private int mPinnedNotificationId = -1;
    private Notification mPinnedNotification;
    private boolean mIsForegroundActive = false;
    private boolean mIsWaitingForForegroundActive = false;

    /**
     * Called by DownloadNotificationStore when there is a change in the list of downloads.
     * @param isActive whether the download is "active" (started/resumed) or not.
     * @param notificationId identifier of the notification associated with the download.
     * @param notification the actual notification associated with the download.
     */
    public void updateDownloadStatus(
            Context context, boolean isActive, int notificationId, Notification notification) {
        if (isActive) {
            mActiveNotifications.put(notificationId, notification);
            if (mPinnedNotificationId < 0) {
                mPinnedNotificationId = notificationId;
                mPinnedNotification = notification;
                startForegroundInternal(context, notificationId, notification);
            }
        } else {
            if (mActiveNotifications.containsKey(notificationId)) {
                mActiveNotifications.remove(notificationId);
                if (mActiveNotifications.isEmpty()) {
                    mPinnedNotificationId = -1;
                    stopForegroundInternal();
                    return;
                }
                if (mPinnedNotificationId == notificationId) {
                    Map.Entry<Integer, Notification> next =
                            mActiveNotifications.entrySet().iterator().next();
                    mPinnedNotificationId = next.getKey();
                    mPinnedNotification = next.getValue();
                    startForegroundInternal(context, next.getKey(), next.getValue());
                }
            }
        }
    }

    @VisibleForTesting
    void startForegroundInternal(Context context, int notificationId, Notification notification) {
        // In case startForeground has been called before or during the starting process.
        if (!mIsForegroundActive && !mIsWaitingForForegroundActive) {
            mIsWaitingForForegroundActive = true;
            Intent intent = new Intent();
            intent.setComponent(new ComponentName(context, DownloadServiceManager.class));
            context.startService(intent);
        }

        // If service is already active, just update the associated notificationId & notification.
        if (mIsForegroundActive) {
            if (!mIsJUnitTest) {
                startForeground(notificationId, notification);
            }
        }
    }

    @VisibleForTesting
    void stopForegroundInternal() {
        // In case this is called before the service has actually been able to start.
        if (mIsForegroundActive) {
            if (!mIsJUnitTest) {
                stopForeground(true /* killNotification */);
            }
            mIsForegroundActive = false;
        }
    }

    @VisibleForTesting
    boolean getIsForegroundActive() {
        return mIsForegroundActive;
    }

    @VisibleForTesting
    int getPinnedNotificationId() {
        return mPinnedNotificationId;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        mIsForegroundActive = true;
        mIsWaitingForForegroundActive = false;

        // In case stopForeground has been called before the service was able to start.
        if (mActiveNotifications.isEmpty()) {
            stopForegroundInternal();
        }

        // Pin the foreground service to the selected notification.
        if (mPinnedNotificationId > 0 && mPinnedNotification != null) {
            if (!mIsJUnitTest) {
                startForeground(mPinnedNotificationId, mPinnedNotification);
            }
        }

        return super.onStartCommand(intent, flags, startId);
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
