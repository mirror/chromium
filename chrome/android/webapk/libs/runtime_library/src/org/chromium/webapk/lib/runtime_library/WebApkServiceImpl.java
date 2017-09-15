// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.lib.runtime_library;

import android.app.AppOpsManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.os.Binder;
import android.os.Build;
import android.os.Bundle;
import android.os.Parcel;
import android.os.RemoteException;

import java.lang.reflect.Field;
import java.lang.reflect.Method;

/**
 * Implements services offered by the WebAPK to Chrome.
 */
public class WebApkServiceImpl extends IWebApkApi.Stub {

    public static final String KEY_SMALL_ICON_ID = "small_icon_id";
    public static final String KEY_HOST_BROWSER_UID = "host_browser_uid";

    private static final String TAG = "WebApkServiceImpl";

    /** Order is same as WebApkNotificationPermissionStatus#enums.xml **/
    private static final int NOTIFICATION_STATUS_DISABLED = 0;
    private static final int NOTIFICATION_STATUS_ENABLED = 1;
    private static final int NOTIFICATION_STATUS_UNKNOWN = 2;

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
        getNotificationManager().notify(platformTag, platformID, notification);
    }

    @Override
    public void cancelNotification(String platformTag, int platformID) {
        getNotificationManager().cancel(platformTag, platformID);
    }

    public int getNotificationStatus() {
        /** Method name on the AppOpsManager class to check for a setting's value. **/
        final String checkOpNoThrow = "checkOpNoThrow";

        /** The POST_NOTIFICATION operation understood by the AppOpsManager. **/
        final String opPostNotification = "OP_POST_NOTIFICATION";

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
            return NOTIFICATION_STATUS_UNKNOWN;
        }

        final String packageName = mContext.getPackageName();
        final int uid = mContext.getApplicationInfo().uid;
        final AppOpsManager appOpsManager =
                (AppOpsManager) mContext.getSystemService(Context.APP_OPS_SERVICE);
        try {
            Class appOpsManagerClass = Class.forName(AppOpsManager.class.getName());

            @SuppressWarnings("unchecked")
            final Method checkOpNoThrowMethod = appOpsManagerClass.getMethod(
                    checkOpNoThrow, Integer.TYPE, Integer.TYPE, String.class);

            final Field opPostNotificationField =
                    appOpsManagerClass.getDeclaredField(opPostNotification);

            int value = (int) opPostNotificationField.get(Integer.class);
            int status = (int) checkOpNoThrowMethod.invoke(appOpsManager, value, uid, packageName);

            if (status == AppOpsManager.MODE_ALLOWED) return NOTIFICATION_STATUS_ENABLED;
            return NOTIFICATION_STATUS_DISABLED;
        } catch (Throwable e) {
        }
        return NOTIFICATION_STATUS_UNKNOWN;
    }

    private NotificationManager getNotificationManager() {
        return (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
    }
}
