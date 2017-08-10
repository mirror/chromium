// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.lib.runtime_library;

import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.os.Binder;
import android.os.Bundle;
import android.os.Parcel;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.util.Log;

import java.io.FileDescriptor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * Implements services offered by the WebAPK to Chrome.
 */
public class WebApkServiceImpl extends IWebApkApi.Stub {

    public static final String KEY_SMALL_ICON_ID = "small_icon_id";
    public static final String KEY_HOST_BROWSER_UID = "host_browser_uid";

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

    /**
     * Creates an instance of WebApkServiceImpl.
     * @param context
     * @param bundle Bundle with additional constructor parameters.
     */
    public WebApkServiceImpl(Context context, Bundle bundle) {
        mContext = context;
        mSmallIconId = bundle.getInt(KEY_SMALL_ICON_ID);
        mHostUid = bundle.getInt(KEY_HOST_BROWSER_UID);
        String nativeLib = context.getApplicationInfo().nativeLibraryDir + "/" + System.mapLibraryName("webapk_native.cr");
        Log.d("Yaron", "libname=" + nativeLib);
        System.load(nativeLib);
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
    public ParcelFileDescriptor createSocket(int family, int type, int protocol) {
        android.util.Log.e("Tag", "socket=" + family + " " + type + " " + protocol);
        int socket = nativeCreateSocket(family, type, protocol);

        android.util.Log.e("Tag", "socket=" + socket);
        return ParcelFileDescriptor.adoptFd(socket);
    }

    @Override
    public void tagSocket(ParcelFileDescriptor descriptor) {
        FileDescriptor fd = descriptor.getFileDescriptor();
        android.util.Log.e("Tag", "tagsocket=" + descriptor.getFd());

        try {
            Class socketTagger = Class.forName("dalvik.system.SocketTagger");
            Method m = socketTagger.getMethod("get");
            Object taggerInst = m.invoke(null);
            android.util.Log.e("Tag", "taggerinst=" + taggerInst);
            m = socketTagger.getMethod("tag", FileDescriptor.class);
            m.setAccessible(true);
            m.invoke(taggerInst, new Object[]{fd});
            android.util.Log.e("Tag", "tagsocket done");
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }


    }



    private NotificationManager getNotificationManager() {
        return (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
    }

    private native int nativeCreateSocket(int family, int type, int protocol);

}
