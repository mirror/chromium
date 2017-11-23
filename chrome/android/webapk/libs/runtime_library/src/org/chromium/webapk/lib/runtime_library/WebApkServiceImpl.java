// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.lib.runtime_library;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.os.Binder;
import android.os.Bundle;
import android.os.Parcel;
import android.os.RemoteException;
import android.support.v4.app.NotificationManagerCompat;

/**
 * Implements services offered by the WebAPK to Chrome.
 */
public class WebApkServiceImpl extends IWebApkApi.Stub {

    public static final String KEY_SMALL_ICON_ID = "small_icon_id";
    public static final String KEY_HOST_BROWSER_UID = "host_browser_uid";

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

    @SuppressWarnings("NewApi")
    @Override
    public void notifyNotification(String platformTag, int platformID, Notification notification) {
        getNotificationManager().notify(platformTag, platformID, notification);
    }

    @Override
    public void cancelNotification(String platformTag, int platformID) {
        getNotificationManager().cancel(platformTag, platformID);
    }

    @Override
    public boolean notificationPermissionEnabled() {
        return NotificationManagerCompat.from(mContext).areNotificationsEnabled();
    }

    @SuppressWarnings("NewApi")
    @Override
    public void notifyNotificationWithChannel(
            String platformTag, int platformID, Notification notification, String channelName) {
        NotificationManager manager = getNotificationManager();
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            // The channel id shouldn't be empty since Chrome always sets the channel id for a
            // notification when running on Android O+ devices.
            ensureNotificationChannelExists(notification.getChannelId(), channelName);
        }
        manager.notify(platformTag, platformID, notification);
    }

    private NotificationManager getNotificationManager() {
        return (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
    }

    @SuppressWarnings("NewApi")
    /** Creates a WebAPK notification channel on Android O+ if one does not exist. */
    private void ensureNotificationChannelExists(String channelId, String channelName) {
        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.O) return;

        NotificationChannel channel = new NotificationChannel(
                channelId, channelName, NotificationManager.IMPORTANCE_DEFAULT);
        getNotificationManager().createNotificationChannel(channel);
    }
}
