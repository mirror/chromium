// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.geolocation;

import android.location.Location;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.webapk.lib.client.WebApkServiceImplClient;
import org.chromium.webapk.lib.runtime_library.ILocationChangedCallback;

/**
 * Created by hanxi on 9/21/17.
 */
@JNINamespace("device")
public class LocationProviderApkAndroid {
    private static final String TAG = "cr_LocationApk";
    private String mPackageName;
    private long mNativePointer;

    public LocationProviderApkAndroid(long nativePtr, String packageName) {
        mNativePointer = nativePtr;
        mPackageName = packageName;
    }

    @CalledByNative
    public static LocationProviderApkAndroid create(long nativePtr, String packageName) {
        return new LocationProviderApkAndroid(nativePtr, packageName);
    }

    @CalledByNative
    public boolean start(boolean highAccuracy) {
        ILocationChangedCallback callback = new ILocationChangedCallback.Stub() {
            @Override
            public void onNewLocationAvailable(Location location) {
                nativeOnNewLocationAvailable(mNativePointer, location.getLatitude(),
                        location.getLongitude(), location.getTime() / 1000.0,
                        location.hasAltitude(), location.getAltitude(), location.hasAccuracy(),
                        location.getAccuracy(), location.hasBearing(), location.getBearing(),
                        location.hasSpeed(), location.getSpeed());
            }
            @Override
            public void newErrorAvailable(String message) {
                nativeOnNewErrorAvailable(mNativePointer, message);
            }
        };
        WebApkServiceImplClient.getInstance().startLocationProvider(
                mPackageName, callback, highAccuracy);
        return true;
    }

    @CalledByNative
    public void stop() {
        WebApkServiceImplClient.getInstance().stopLocationProvider(mPackageName);
    }

    @CalledByNative
    public boolean isRunning() {
        return WebApkServiceImplClient.getInstance().isLocationProviderRunning(mPackageName);
    }

    public void newErrorAvailable(String message) {
        Log.e(TAG, "newErrorAvailable %s", message);
        nativeOnNewErrorAvailable(mNativePointer, message);
    }

    // Native functions
    private native void nativeOnNewLocationAvailable(long nativeLocationProviderApkAndroid,
            double latitude, double longitude, double timeStamp, boolean hasAltitude,
            double altitude, boolean hasAccuracy, double accuracy, boolean hasHeading,
            double heading, boolean hasSpeed, double speed);
    private native void nativeOnNewErrorAvailable(
            long nativeLocationProviderApkAndroid, String message);
}
