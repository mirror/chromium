// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.shell_apk;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Parcel;
import android.os.RemoteException;
import android.util.Log;

import org.chromium.webapk.lib.runtime_library.IWebApkApi;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
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
     * Name of the class with IBinder API implementation.
     */
    private static final String WEBAPK_SERVICE_IMPL_CLASS_NAME =
            "org.chromium.webapk.lib.runtime_library.WebApkServiceImpl";

    /**
     * Key for passing id of icon to represent WebAPK notifications in status bar.
     */
    private static final String KEY_SMALL_ICON_ID = "small_icon_id";

    /**
     * Key for passing uid of only application allowed to call the service's methods.
     */
    private static final String KEY_HOST_BROWSER_UID = "host_browser_uid";

    private IBinder mIBinderDelegate;
    private Context mContext;

    public WebApkServiceImplWrapper(Context context) {
        mContext = context;
        ClassLoader webApkClassLoader = HostBrowserClassLoader.getClassLoaderInstance(
                mContext, WEBAPK_SERVICE_IMPL_CLASS_NAME);
        if (webApkClassLoader == null) {
            Log.w(TAG, "Unable to create ClassLoader.");
            return;
        }

        try {
            Class<?> webApkServiceImplClass =
                    webApkClassLoader.loadClass(WEBAPK_SERVICE_IMPL_CLASS_NAME);
            Constructor<?> webApkServiceImplConstructor =
                    webApkServiceImplClass.getConstructor(Context.class, Bundle.class);
            int hostBrowserUid = WebApkUtils.getHostBrowserUid(mContext);
            Bundle bundle = new Bundle();
            bundle.putInt(KEY_SMALL_ICON_ID, R.drawable.notification_badge);
            bundle.putInt(KEY_HOST_BROWSER_UID, hostBrowserUid);
            mIBinderDelegate = (IBinder) webApkServiceImplConstructor.newInstance(
                    new Object[] {mContext, bundle});
        } catch (Exception e) {
            Log.w(TAG, "Unable to create WebApkServiceImpl.");
            e.printStackTrace();
            return;
        }

        createNotificationChannelIfNotExist();
    }

    @Override
    public boolean onTransact(int arg0, Parcel arg1, Parcel arg2, int arg3) throws RemoteException {
        int code = getApiCode();

        if (arg0 == code) {
            // This is a notifyNotification call, so defer to our parent's onTransact which will
            // eventually dispatch it to our notifyNotification.
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
        } catch (NoSuchFieldException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
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
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
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
        } catch (NoSuchFieldException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
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
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        }
    }
}
