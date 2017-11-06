// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.lib.client;

import android.content.Context;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import org.chromium.webapk.lib.runtime_library.ILocationChangedCallback;
import org.chromium.webapk.lib.runtime_library.IPermissionRequestCallback;
import org.chromium.webapk.lib.runtime_library.IWebApkApi;

/**
 * Created by hanxi on 9/25/17.
 */
public class WebApkServiceImplClient {
    /** Callbacks when the connection to the WebAPK service is established. */
    public interface ConnectionCallback {
        /** Called when the connection is established. */
        void onConnected(IWebApkApi api);
    }

    /** Callback which catches RemoteExceptions thrown due to IWebApkApi failure. */
    private abstract static class ApiUseCallback
            implements WebApkServiceConnectionManager.ConnectionCallback {
        public abstract void useApi(IWebApkApi api) throws RemoteException;

        @Override
        public void onConnected(IBinder api) {
            if (api == null) return;

            try {
                useApi(IWebApkApi.Stub.asInterface(api));
            } catch (RemoteException e) {
                Log.w(TAG, "WebApkAPI use failed.", e);
            }
        }
    }

    private static final String CATEGORY_WEBAPK_API = "android.intent.category.WEBAPK_API";
    private static final String TAG = "cr_WebApk";

    private static WebApkServiceImplClient sInstance;

    /** Manages connections between the browser application and WebAPK services. */
    private WebApkServiceConnectionManager mConnectionManager;

    public static WebApkServiceImplClient getInstance() {
        if (sInstance == null) {
            sInstance = new WebApkServiceImplClient();
        }
        return sInstance;
    }

    private WebApkServiceImplClient() {
        mConnectionManager =
                new WebApkServiceConnectionManager(CATEGORY_WEBAPK_API, null /* action */);
    }

    /**
     * Connects to a WebAPK's bound service, builds a notification and hands it over to the WebAPK
     * to display. Handing over the notification makes the notification look like it originated from
     * the WebAPK - not Chrome - in the Android UI.
     */
    public void notifyNotification(Context context, final String webApkPackage,
            final String platformTag, final int platformID, final ConnectionCallback callback) {
        final ApiUseCallback connectionCallback = new WebApkServiceImplClient.ApiUseCallback() {
            @Override
            public void useApi(IWebApkApi api) throws RemoteException {
                callback.onConnected(api);
            }
        };

        mConnectionManager.connect(context, webApkPackage, connectionCallback);
    }

    /** Cancels notification previously shown by WebAPK. */
    public void cancelNotification(
            Context context, String webApkPackage, final String platformTag, final int platformID) {
        final ApiUseCallback connectionCallback = new WebApkServiceImplClient.ApiUseCallback() {
            @Override
            public void useApi(IWebApkApi api) throws RemoteException {
                api.cancelNotification(platformTag, platformID);
            }
        };

        mConnectionManager.connect(context, webApkPackage, connectionCallback);
    }

    public void requestPermission(Context context, String webApkPackage,
            final IPermissionRequestCallback callback, final String[] permissions) {
        final ApiUseCallback connectionCallback = new ApiUseCallback() {
            @Override
            public void useApi(IWebApkApi api) throws RemoteException {
                api.requestPermission(callback, permissions);
            }
        };

        mConnectionManager.connect(context, webApkPackage, connectionCallback);
    }

    public void startLocationProvider(Context context, String webApkPackage,
            final ILocationChangedCallback callback, final boolean enableHighAccuracy) {
        final ApiUseCallback connectionCallback = new WebApkServiceImplClient.ApiUseCallback() {
            @Override
            public void useApi(IWebApkApi api) throws RemoteException {
                api.startLocationProvider(callback, enableHighAccuracy);
            }
        };

        mConnectionManager.connect(context, webApkPackage, connectionCallback);
    }

    public void stopLocationProvider(Context context, String webApkPackage) {
        final ApiUseCallback connectionCallback = new WebApkServiceImplClient.ApiUseCallback() {
            @Override
            public void useApi(IWebApkApi api) throws RemoteException {
                api.stopLocationProvider();
            }
        };

        mConnectionManager.connect(context, webApkPackage, connectionCallback);
    }

    public boolean isLocationProviderRunning(String webApkPackage) {
        IBinder service = mConnectionManager.getService(webApkPackage);
        if (service == null) return false;

        IWebApkApi api = IWebApkApi.Stub.asInterface(service);
        try {
            return api.isLocationProviderRunning();
        } catch (RemoteException e) {
            return false;
        }
    }

    /** Disconnects all the connections to WebAPK services.
     * @param context*/
    public static void disconnectAll(Context context) {
        if (sInstance == null) return;

        sInstance.mConnectionManager.disconnectAll(context);
    }
}
