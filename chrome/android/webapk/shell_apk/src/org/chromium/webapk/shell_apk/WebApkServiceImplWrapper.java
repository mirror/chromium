// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.shell_apk;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.os.Binder;
import android.os.IBinder;
import android.os.Parcel;
import android.os.RemoteException;
import android.util.Log;

import org.chromium.webapk.lib.runtime_library.IWebApkApi;

import java.lang.reflect.Field;
import java.lang.reflect.Method;

/**
 * A wrapper class of {@link org.chromium.webapk.lib.runtime_library.WebApkServiceImpl} that
 * provides additional functionality when the runtime library hasn't been updated. Extracts code
 * with {@link org.chromium.webapk.lib.runtime_library.WebApkServiceImpl} from .dex file in Chrome
 * APK.
 */
public class WebApkServiceImplWrapper extends IWebApkApi.Stub {
    private static final String TAG = "cr_WebApkService";

    /** The channel id of the WebAPK. */
    private static final String NOTIFICATION_CHANNEL_ID = "General";

    /**
     * Uid of only application allowed to call the service's methods. If an application with a
     * different uid calls the service, the service throws a RemoteException.
     */
    private final int mHostUid;

    private IBinder mIBinderDelegate;
    private Context mContext;

    public WebApkServiceImplWrapper(Context context, IBinder delegate, int hostBrowserUid) {
        mContext = context;
        mIBinderDelegate = delegate;
        mHostUid = hostBrowserUid;
        createNotificationChannelIfNotExist();
    }

    @Override
    public boolean onTransact(int arg0, Parcel arg1, Parcel arg2, int arg3) throws RemoteException {
        int code = getApiCode();

        if (arg0 == code) {
            // This is a notifyNotification call, so defer to our parent's onTransact which will
            // eventually dispatch it to our notifyNotification.
            int callingUid = Binder.getCallingUid();
            if (mHostUid != callingUid) {
                throw new RemoteException("Unauthorized caller " + callingUid
                        + " does not match expected host=" + mHostUid);
            }
            return super.onTransact(arg0, arg1, arg2, arg3);
        }

        return delegateOnTransactMethod(arg0, arg1, arg2, arg3);
    }

    @Override
    public int getSmallIconId() {
        Log.w(TAG, "Should NOT reach WebApkServiceImplWrapper#getSmallIconId().");
        return -1;
    }

    @Override
    public boolean notificationPermissionEnabled() throws RemoteException {
        Log.w(TAG, "Should NOT reach WebApkServiceImplWrapper#notificationPermissionEnabled().");
        return false;
    }

    @Override
    @SuppressWarnings("NewApi")
    public void notifyNotification(String platformTag, int platformID, Notification notification) {
        // We always rewrite the notification channel name to the WebAPK's channel name on O+.
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            if (!setNotificationChannelId(notification)) return;
        }
        delegateNotifyNotification(platformTag, platformID, notification);
    }

    @Override
    public void cancelNotification(String platformTag, int platformID) {
        Log.w(TAG, "Should NOT reach WebApkServiceImplWrapper#cancelNotification(String, int).");
    }

    /** Creates a WebAPK notification channel on Android O+ if hasn't. */
    private void createNotificationChannelIfNotExist() {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            NotificationManager manager =
                    (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
            if (manager.getNotificationChannel(NOTIFICATION_CHANNEL_ID) != null) return;

            NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL_ID,
                    mContext.getString(R.string.notification_channel_name),
                    NotificationManager.IMPORTANCE_DEFAULT);
            manager.createNotificationChannel(channel);
        }
    }

    private int getApiCode() {
        if (mIBinderDelegate == null) return -1;

        try {
            Field f = mIBinderDelegate.getClass().getSuperclass().getDeclaredField(
                    "TRANSACTION_notifyNotification");
            f.setAccessible(true);
            return (int) f.get(null);
        } catch (Exception e) {
            e.printStackTrace();
        }

        return -1;
    }

    private boolean delegateOnTransactMethod(int arg0, Parcel arg1, Parcel arg2, int arg3)
            throws RemoteException {
        if (mIBinderDelegate == null) return false;

        try {
            Method onTransactMethod = mIBinderDelegate.getClass().getMethod(
                    "onTransact", new Class[] {int.class, Parcel.class, Parcel.class, int.class});
            onTransactMethod.setAccessible(true);
            return (boolean) onTransactMethod.invoke(mIBinderDelegate, arg0, arg1, arg2, arg3);
        } catch (Exception e) {
            e.printStackTrace();
        }

        return false;
    }

    private boolean setNotificationChannelId(Notification notification) {
        if (notification == null) return false;

        try {
            Field channelId = notification.getClass().getDeclaredField("mChannelId");
            channelId.setAccessible(true);
            channelId.set(notification, mContext.getString(R.string.notification_channel_name));
            return true;
        } catch (Exception e) {
            e.printStackTrace();
        }

        return false;
    }

    private void delegateNotifyNotification(
            String platformTag, int platformID, Notification notification) {
        if (mIBinderDelegate == null) return;

        try {
            Method notifyMethod = mIBinderDelegate.getClass().getMethod("notifyNotification",
                    new Class[] {String.class, int.class, Notification.class});
            notifyMethod.setAccessible(true);
            notifyMethod.invoke(mIBinderDelegate, platformTag, platformID, notification);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
