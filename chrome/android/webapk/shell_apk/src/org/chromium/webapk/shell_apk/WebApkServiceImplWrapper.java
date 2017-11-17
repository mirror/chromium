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
 * provides additional functionality when the runtime library hasn't updated.
 */
public class WebApkServiceImplWrapper extends IWebApkApi.Stub {
    private static final String TAG = "cr_WebApkServiceFactory";

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

    private IBinder mDelegate;
    private Context mContext;
    private ClassLoader mWebApkClassLoader;

    public WebApkServiceImplWrapper(Context context) {
        mContext = context;
        mWebApkClassLoader = HostBrowserClassLoader.getClassLoaderInstance(
                mContext, WEBAPK_SERVICE_IMPL_CLASS_NAME);
        if (mWebApkClassLoader == null) {
            Log.w(TAG, "Unable to create ClassLoader.");
            return;
        }

        try {
            Class<?> webApkServiceImplClass =
                    mWebApkClassLoader.loadClass(WEBAPK_SERVICE_IMPL_CLASS_NAME);
            Constructor<?> webApkServiceImplConstructor =
                    webApkServiceImplClass.getConstructor(Context.class, Bundle.class);
            int hostBrowserUid = WebApkUtils.getHostBrowserUid(mContext);
            Bundle bundle = new Bundle();
            bundle.putInt(KEY_SMALL_ICON_ID, R.drawable.notification_badge);
            bundle.putInt(KEY_HOST_BROWSER_UID, hostBrowserUid);
            mDelegate = (IBinder) webApkServiceImplConstructor.newInstance(
                    new Object[] {mContext, bundle});
        } catch (Exception e) {
            Log.w(TAG, "Unable to create WebApkServiceImpl.");
            e.printStackTrace();
            return;
        }

        // Creates a notification channel on Android O+.
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

    @Override
    public boolean onTransact(int arg0, Parcel arg1, Parcel arg2, int arg3) throws RemoteException {
        int code = 0;
        try {
            Field f = mDelegate.getClass().getSuperclass().getDeclaredField(
                    "TRANSACTION_notifyNotification");
            f.setAccessible(true);
            code = (int) f.get(null);
        } catch (NoSuchFieldException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        }

        if (arg0 == code) {
            return super.onTransact(arg0, arg1, arg2, arg3);
        }

        try {
            Method method = mDelegate.getClass().getMethod(
                    "onTransact", new Class[] {int.class, Parcel.class, Parcel.class, int.class});
            method.setAccessible(true);
            return (boolean) method.invoke(mDelegate, arg0, arg1, arg2, arg3);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        }
        return false;
    }

    @Override
    public int getSmallIconId() {
        return -1;
    }

    @Override
    public boolean notificationPermissionEnabled() throws RemoteException {
        return false;
    }

    @Override
    @SuppressWarnings("NewApi")
    public void notifyNotification(String platformTag, int platformID, Notification notification) {
        NotificationManager manager =
                (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        // We always rewrite the notification channel name to the WebAPK's channel name on O+.
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            try {
                Field f = notification.getClass().getDeclaredField("mChannelId");
                f.setAccessible(true);
                f.set(notification, mContext.getString(R.string.notification_channel_name));
            } catch (NoSuchFieldException e) {
                e.printStackTrace();
            } catch (IllegalAccessException e) {
                e.printStackTrace();
            }
        }
        manager.notify(platformTag, platformID, notification);
    }

    @Override
    public void cancelNotification(String platformTag, int platformID) {}
}
