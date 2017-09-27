// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.lib.runtime_library;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.location.Location;
import android.os.Build;
import android.os.Bundle;
import android.os.Looper;
import android.os.RemoteException;
import android.util.Log;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.common.api.GoogleApiClient.ConnectionCallbacks;
import com.google.android.gms.common.api.GoogleApiClient.OnConnectionFailedListener;
import com.google.android.gms.location.FusedLocationProviderApi;
import com.google.android.gms.location.LocationListener;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationServices;

/**
 * This is a LocationProvider using Google Play Services.
 *
 * https://developers.google.com/android/reference/com/google/android/gms/location/package-summary
 */
public class LocationProviderGmsCore
        implements ConnectionCallbacks, OnConnectionFailedListener, LocationListener {
    private static final String TAG = "cr_LocationProvider";

    // Values for the LocationRequest's setInterval for normal and high accuracy, respectively.
    private static final long UPDATE_INTERVAL_MS = 1000;
    private static final long UPDATE_INTERVAL_FAST_MS = 500;

    private static LocationProviderGmsCore sProvider;
    private final GoogleApiClient mGoogleApiClient;
    private FusedLocationProviderApi mLocationProviderApi = LocationServices.FusedLocationApi;

    private ILocationChangedCallback mCallback;

    private boolean mEnablehighAccuracy;
    private LocationRequest mLocationRequest;

    public static boolean isGooglePlayServicesAvailable(Context context) {
        return GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(context)
                == ConnectionResult.SUCCESS;
    }

    public static LocationProviderGmsCore getInstance(Context context) {
        if (sProvider == null) {
            sProvider = new LocationProviderGmsCore(context);
        }
        return sProvider;
    }

    LocationProviderGmsCore(Context context) {
        Log.i(TAG, "Google Play Services");
        mGoogleApiClient = new GoogleApiClient.Builder(context)
                                   .addApi(LocationServices.API)
                                   .addConnectionCallbacks(this)
                                   .addOnConnectionFailedListener(this)
                                   .build();
        assert mGoogleApiClient != null;
    }

    // ConnectionCallbacks implementation
    @Override
    public void onConnected(Bundle connectionHint) {
        mLocationRequest = LocationRequest.create();
        if (mEnablehighAccuracy) {
            mLocationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY)
                    .setInterval(UPDATE_INTERVAL_FAST_MS);
        } else {
            mLocationRequest.setPriority(LocationRequest.PRIORITY_BALANCED_POWER_ACCURACY)
                    .setInterval(UPDATE_INTERVAL_MS);
        }

        final Location location = mLocationProviderApi.getLastLocation(mGoogleApiClient);
        if (location != null) {
            try {
                mCallback.onNewLocationAvailable(location);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        Context context = mGoogleApiClient.getContext();
        boolean hasPermission = (Build.VERSION.SDK_INT < Build.VERSION_CODES.M)
                || ((mEnablehighAccuracy
                            && context.checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION)
                                    == PackageManager.PERMISSION_GRANTED)
                           || (!mEnablehighAccuracy
                                      && context.checkSelfPermission(
                                                 android.Manifest.permission.ACCESS_COARSE_LOCATION)
                                              == PackageManager.PERMISSION_GRANTED));
        if (hasPermission) {
            try {
                // Request updates on UI Thread replicating LocationProviderAndroid's behaviour.
                mLocationProviderApi.requestLocationUpdates(
                        mGoogleApiClient, mLocationRequest, this, Looper.getMainLooper());
            } catch (IllegalStateException | SecurityException e) {
                // IllegalStateException is thrown "If this method is executed in a thread that has
                // not called Looper.prepare()". SecurityException is thrown if there is no
                // permission, see https://crbug.com/731271.
                Log.e(TAG, " mLocationProviderApi.requestLocationUpdates() " + e);
                try {
                    mCallback.newErrorAvailable(
                            "Failed to request location updates: " + e.toString());
                } catch (RemoteException e1) {
                }
            }
        } else {
            try {
                mCallback.newErrorAvailable(
                        "Failed to connect to Google Play Services: no permission");
            } catch (RemoteException e) {
            }
        }
    }

    @Override
    public void onConnectionSuspended(int cause) {}

    // OnConnectionFailedListener implementation
    @Override
    public void onConnectionFailed(ConnectionResult result) {
        try {
            mCallback.newErrorAvailable(
                    "Failed to connect to Google Play Services: " + result.toString());
        } catch (RemoteException e) {
        }
    }

    // LocationProviderFactory.LocationProvider implementation
    public void start(ILocationChangedCallback callback, boolean enableHighAccuracy) {
        if (mGoogleApiClient.isConnected()) mGoogleApiClient.disconnect();
        mCallback = callback;

        mEnablehighAccuracy = enableHighAccuracy;
        mGoogleApiClient.connect(); // Should return via onConnected().
    }

    public void stop() {
        if (!mGoogleApiClient.isConnected()) return;

        mLocationProviderApi.removeLocationUpdates(mGoogleApiClient, this);

        mGoogleApiClient.disconnect();
        mCallback = null;
    }

    public boolean isRunning() {
        if (mGoogleApiClient == null) return false;
        return mGoogleApiClient.isConnecting() || mGoogleApiClient.isConnected();
    }

    // LocationListener implementation
    public void onLocationChanged(Location location) {
        try {
            mCallback.onNewLocationAvailable(location);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }
}
