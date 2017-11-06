// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.lib.runtime_library;

import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.Parcel;
import android.os.RemoteException;
import android.support.v4.app.NotificationManagerCompat;

/**
 * Implements services offered by the WebAPK to Chrome.
 */
public class WebApkServiceImpl extends IWebApkApi.Stub {
    public static final String KEY_SMALL_ICON_ID = "small_icon_id";
    public static final String KEY_HOST_BROWSER_UID = "host_browser_uid";
    public static final String EXTRA_PERMISSIONS = "extra.permissions";
    public static final String EXTRA_GRANT_RESULTS = "extra.granted.results";
    public static final String EXTRA_RESULT_RECEIVER = "extra.result.receiver";

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

    private boolean mIsProviderGmsCore;

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

    @Override
    public void startLocationProvider(
            final ILocationChangedCallback callback, final boolean enableHighAccuracy) {
        mIsProviderGmsCore = LocationProviderGmsCore.isGooglePlayServicesAvailable(mContext);
        Handler mainHandler = new Handler(Looper.getMainLooper());
        Runnable myRunnable = new Runnable() {
            @Override
            public void run() {
                if (mIsProviderGmsCore) {
                    LocationProviderGmsCore.getInstance(mContext).start(
                            callback, enableHighAccuracy);
                } else {
                    LocationProviderAndroid.getInstance().start(
                            mContext, callback, enableHighAccuracy);
                }
            }
        };
        mainHandler.post(myRunnable);
    }

    @Override
    public void stopLocationProvider() {
        Handler mainHandler = new Handler(Looper.getMainLooper());
        Runnable myRunnable = new Runnable() {
            @Override
            public void run() {
                if (mIsProviderGmsCore) {
                    LocationProviderGmsCore.getInstance(mContext).stop();
                } else {
                    LocationProviderAndroid.getInstance().stop();
                }
            }
        };
        mainHandler.post(myRunnable);
    }

    @Override
    public boolean isLocationProviderRunning() {
        if (mIsProviderGmsCore) {
            return LocationProviderGmsCore.getInstance(mContext).isRunning();
        }
        return LocationProviderAndroid.getInstance().isRunning();
    }

    @Override
    public void requestPermission(
            final IPermissionRequestCallback callback, final String[] permissions) {
        Handler handler = new Handler(Looper.getMainLooper(), new Handler.Callback() {
            @Override
            public boolean handleMessage(Message message) {
                Bundle data = message.getData();
                final int[] results = data.getIntArray(EXTRA_GRANT_RESULTS).clone();
                try {
                    callback.onRequestPermissionsResult(results);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
                return true;
            }
        });
        final Messenger messenger = new Messenger(handler);
        handler.post(new Runnable() {
            @Override
            public void run() {
                Intent intent = new Intent();
                intent.setClassName(mContext, "org.chromium.webapk.shell_apk.PermissionActivity");
                intent.putExtra(EXTRA_PERMISSIONS, permissions);
                intent.putExtra(EXTRA_RESULT_RECEIVER, messenger);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
                mContext.startActivity(intent);
            }
        });
    }

    public boolean notificationPermissionEnabled() {
        return NotificationManagerCompat.from(mContext).areNotificationsEnabled();
    }

    private NotificationManager getNotificationManager() {
        return (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
    }
}
