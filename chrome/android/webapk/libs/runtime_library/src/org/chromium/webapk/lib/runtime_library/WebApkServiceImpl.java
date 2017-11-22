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

import java.lang.reflect.Field;

/**
 * Implements services offered by the WebAPK to Chrome.
 */
public class WebApkServiceImpl extends IWebApkApi.Stub {

    public static final String KEY_SMALL_ICON_ID = "small_icon_id";
    public static final String KEY_HOST_BROWSER_UID = "host_browser_uid";
    public static final String KEY_DEFAULT_CHANNEL_NAME = "default_channel_name";
    public static final String KEY_TARGET_SDK = "target_sdk";

    /** The default channel id of the WebAPK. */
    private static final String DEFAULT_NOTIFICATION_CHANNEL_ID = "default_channel_id";

    private static final String TAG = "WebApkServiceImpl";

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

    /** The default channel name of a WebAPK. */
    private final String mChannelName;

    /** The target SDK of the owner WebAPK. */
    private int mTargetSdk = 23;

    /**
     * Creates an instance of WebApkServiceImpl.
     * @param context
     * @param bundle Bundle with additional constructor parameters.
     */
    public WebApkServiceImpl(Context context, Bundle bundle) {
        mContext = context;
        mSmallIconId = bundle.getInt(KEY_SMALL_ICON_ID);
        mChannelName = bundle.getString(KEY_DEFAULT_CHANNEL_NAME);
        mHostUid = bundle.getInt(KEY_HOST_BROWSER_UID);
        mTargetSdk = bundle.getInt(KEY_TARGET_SDK);
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
        NotificationManager manager = getNotificationManager();
        // We always rewrite the notification channel id to the WebAPK's default channel id on O+ if
        // WebAPK targets to SDK 26+.
        if (supportNotificationChannel()) {
            ensureNotificationChannelExists(DEFAULT_NOTIFICATION_CHANNEL_ID, mChannelName);
            if (!setNotificationChannelId(notification)) return;
        }
        getNotificationManager().notify(platformTag, platformID, notification);
    }

    @SuppressWarnings("NewApi")
    @Override
    public void notifyNotificationWithChannel(
            String platformTag, int platformID, Notification notification, String channelName) {
        NotificationManager manager = getNotificationManager();
        // We always rewrite the notification channel id to the WebAPK's default channel id on O+.
        if (supportNotificationChannel()) {
            String channelId = notification.getChannelId();
            if (channelId == null) {
                ensureNotificationChannelExists(DEFAULT_NOTIFICATION_CHANNEL_ID, mChannelName);
                if (!setNotificationChannelId(notification)) return;
            } else {
                ensureNotificationChannelExists(channelId, channelName);
            }
        }
        manager.notify(platformTag, platformID, notification);
    }

    @Override
    public void cancelNotification(String platformTag, int platformID) {
        getNotificationManager().cancel(platformTag, platformID);
    }

    @Override
    public boolean notificationPermissionEnabled() {
        return NotificationManagerCompat.from(mContext).areNotificationsEnabled();
    }

    private NotificationManager getNotificationManager() {
        return (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
    }

    private boolean supportNotificationChannel() {
        return (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O)
                && mTargetSdk >= android.os.Build.VERSION_CODES.O;
    }

    @SuppressWarnings("NewApi")
    /** Creates a WebAPK notification channel on Android O+ if one does not exist. */
    private void ensureNotificationChannelExists(String channelId, String channelName) {
        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.O) return;

        NotificationChannel channel = new NotificationChannel(
                channelId, channelName, NotificationManager.IMPORTANCE_DEFAULT);
        getNotificationManager().createNotificationChannel(channel);
    }

    /** Sets the notification channel of the given notification object via reflection. */
    private boolean setNotificationChannelId(Notification notification) {
        try {
            // Before WebAPKs target to SDK 26, there isn't a channel id is set when the
            // notification is built by Chrome. We need to set a channel id to display the
            // notification properly.
            Field channelId = notification.getClass().getDeclaredField("mChannelId");
            channelId.setAccessible(true);
            channelId.set(notification, DEFAULT_NOTIFICATION_CHANNEL_ID);
            return true;
        } catch (Exception e) {
            e.printStackTrace();
        }

        return false;
    }
}
