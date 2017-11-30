// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.lib.runtime_library;

import android.annotation.TargetApi;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.os.Binder;
import android.os.Build;
import android.os.Bundle;
import android.os.Parcel;
import android.os.RemoteException;
import android.support.v4.app.NotificationManagerCompat;
import android.util.Log;

/**
 * Implements services offered by the WebAPK to Chrome.
 */
public class WebApkServiceImpl extends IWebApkApi.Stub {

    public static final String KEY_SMALL_ICON_ID = "small_icon_id";
    public static final String KEY_HOST_BROWSER_UID = "host_browser_uid";

    private static final String TAG = "cr_WebApkServiceImpl";

    /**
     * Keeps this value consistent with {@link
     * org.chromium.webapk.shell_apk.WebApkServiceImplWrapper#DEFAULT_NOTIFICATION_CHANNEL_ID}.
     */
    private static final String LEGACY_CHANNEL_ID = "default_channel_id";

    private final Context mContext;

    /**
     * Id of icon to represent WebAPK notifications in status bar.
     */
    private final int mSmallIconId;

    /**
     * Uid of only application allowed to call the service's methods. If an application with a
     * different uid calls the service, the service throws a RemoteException.
     */
    private final int mHostUid;

    /**
     * Creates an instance of WebApkServiceImpl.
     * @param context
     * @param bundle Bundle with additional constructor parameters.
     */
    public WebApkServiceImpl(Context context, Bundle bundle) {
        mContext = context;
        mSmallIconId = bundle.getInt(KEY_SMALL_ICON_ID);
        mHostUid = bundle.getInt(KEY_HOST_BROWSER_UID);
        assert mHostUid >= 0;
    }

    @Override
    public boolean onTransact(int arg0, Parcel arg1, Parcel arg2, int arg3) throws RemoteException {
        int callingUid = Binder.getCallingUid();
        if (mHostUid != callingUid) {
            throw new RemoteException("Unauthorized caller " + callingUid
                    + " does not match expected host=" + mHostUid);
        }
        return super.onTransact(arg0, arg1, arg2, arg3);
    }

    @Override
    public int getSmallIconId() {
        return mSmallIconId;
    }

    @Override
    public void notifyNotification(String platformTag, int platformID, Notification notification) {
        Log.w(TAG,
                "Should NOT reach WebApkServiceImpl#notifyNotification(String, int,"
                        + " Notification).");
    }

    @Override
    public void cancelNotification(String platformTag, int platformID) {
        getNotificationManager().cancel(platformTag, platformID);
    }

    @Override
    public boolean notificationPermissionEnabled() {
        return NotificationManagerCompat.from(mContext).areNotificationsEnabled();
    }

    @Override
    public void notifyNotificationWithChannel(
            String platformTag, int platformID, Notification notification, String channelName) {
        NotificationManager notificationManager = getNotificationManager();
        if (usingChannels(notification)) {
            // crbug.com/700228. We used to create a default notification channel for WebAPKs which
            // target to SDK 26 when Chrome doesn't send notifications with channel ID to WebAPKs.
            // See {@link WebApkServiceImplWrapper#notifyNotification()} for details.
            cleanUpLegacyChannel(notificationManager);
            createNotificationChannel(notificationManager, notification, channelName);
        }
        notificationManager.notify(platformTag, platformID, notification);
    }

    private NotificationManager getNotificationManager() {
        return (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
    }

    /** Returns whether a channel ID has been set for the given notification. */
    @TargetApi(Build.VERSION_CODES.O)
    private boolean usingChannels(Notification notification) {
        return notification.getChannelId() != null;
    }

    /** Creates a notification channel on Android O+. */
    @TargetApi(Build.VERSION_CODES.O)
    private void createNotificationChannel(NotificationManager notificationManager,
            Notification notification, String channelName) {
        // The channel id shouldn't be empty since Chrome always sets the channel id for a
        // notification when WebAPK targets to SDK 26+.
        NotificationChannel channel = new NotificationChannel(
                notification.getChannelId(), channelName, NotificationManager.IMPORTANCE_DEFAULT);
        notificationManager.createNotificationChannel(channel);
    }

    /** Deletes the legacy notification channel. */
    @TargetApi(Build.VERSION_CODES.O)
    private void cleanUpLegacyChannel(NotificationManager notificationManager) {
        notificationManager.deleteNotificationChannel(LEGACY_CHANNEL_ID);
    }
}