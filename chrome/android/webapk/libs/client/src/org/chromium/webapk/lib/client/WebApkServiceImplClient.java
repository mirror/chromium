// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.lib.client;

import android.os.IBinder;
import android.os.RemoteException;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.webapk.lib.runtime_library.ILocationChangedCallback;
import org.chromium.webapk.lib.runtime_library.IWebApkApi;

/**
 * Created by hanxi on 9/25/17.
 */

public class WebApkServiceImplClient {
    public interface ConnectionCallback { void onConnected(IWebApkApi api); }

    // Callback which catches RemoteExceptions thrown due to IWebApkApi failure.
    private abstract static class ApiUseCallback
            implements WebApkServiceConnectionManager.ConnectionCallback {
        public abstract void useApi(IWebApkApi api) throws RemoteException;

        @Override
        public void onConnected(IBinder api) {
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
    public void notifyNotification(final String webApkPackage, final String platformTag,
            final int platformID, final ConnectionCallback callback) {
        final WebApkServiceImplClient.ApiUseCallback connectionCallback =
                new WebApkServiceImplClient.ApiUseCallback() {
                    @Override
                    public void useApi(IWebApkApi api) throws RemoteException {
                        callback.onConnected(api);
                    }
                };

        mConnectionManager.connect(
                ContextUtils.getApplicationContext(), webApkPackage, connectionCallback);
    }

    /** Cancels notification previously shown by WebAPK. */
    public void cancelNotification(
            String webApkPackage, final String platformTag, final int platformID) {
        final WebApkServiceImplClient.ApiUseCallback connectionCallback =
                new WebApkServiceImplClient.ApiUseCallback() {
                    @Override
                    public void useApi(IWebApkApi api) throws RemoteException {
                        api.cancelNotification(platformTag, platformID);
                    }
                };

        mConnectionManager.connect(
                ContextUtils.getApplicationContext(), webApkPackage, connectionCallback);
    }

    public void startLocationProvider(String webApkPackage, final ILocationChangedCallback callback,
            final boolean enableHighAccuracy) {
        final WebApkServiceImplClient.ApiUseCallback connectionCallback =
                new WebApkServiceImplClient.ApiUseCallback() {
                    @Override
                    public void useApi(IWebApkApi api) throws RemoteException {
                        api.startLocationProvider(callback, enableHighAccuracy);
                    }
                };

        mConnectionManager.connect(
                ContextUtils.getApplicationContext(), webApkPackage, connectionCallback);
    }

    public void stopLocationProvider(String webApkPackage) {
        final WebApkServiceImplClient.ApiUseCallback connectionCallback =
                new WebApkServiceImplClient.ApiUseCallback() {
                    @Override
                    public void useApi(IWebApkApi api) throws RemoteException {
                        api.stopLocationProvider();
                    }
                };

        mConnectionManager.connect(
                ContextUtils.getApplicationContext(), webApkPackage, connectionCallback);
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

    /** Disconnects all the connections to WebAPK services. */
    public static void disconnectAll() {
        if (sInstance == null) return;

        sInstance.mConnectionManager.disconnectAll(ContextUtils.getApplicationContext());
    }
}
