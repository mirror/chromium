// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.lib.runtime_library;

import android.content.Context;
import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.os.Looper;
import android.os.RemoteException;
import android.util.Log;

import java.util.List;

/**
 * This is a LocationProvider using Android APIs [1]. It is a separate class for clarity
 * so that it can manage all processing completely on the UI thread. The container class
 * ensures that the start/stop calls into this class are done on the UI thread.
 *
 * [1] https://developer.android.com/reference/android/location/package-summary.html
 */
public class LocationProviderAndroid implements LocationListener {
    private static final String TAG = "cr_LocationProvider";

    private LocationManager mLocationManager;
    private boolean mIsRunning;
    private ILocationChangedCallback mCallback;
    private static LocationProviderAndroid sProvider;

    public static LocationProviderAndroid getInstance() {
        if (sProvider == null) {
            sProvider = new LocationProviderAndroid();
        }
        return sProvider;
    }

    LocationProviderAndroid() {}

    public void start(
            Context context, ILocationChangedCallback callback, boolean enableHighAccuracy) {
        unregisterFromLocationUpdates();
        mCallback = callback;
        registerForLocationUpdates(context, enableHighAccuracy);
    }

    public void stop() {
        unregisterFromLocationUpdates();
    }

    public boolean isRunning() {
        return mIsRunning;
    }

    @Override
    public void onLocationChanged(Location location) {
        // Callbacks from the system location service are queued to this thread, so it's
        // possible that we receive callbacks after unregistering. At this point, the
        // native object will no longer exist.
        if (mIsRunning) {
            try {
                mCallback.onNewLocationAvailable(location);
            } catch (RemoteException e) {
            }
        }
    }

    @Override
    public void onStatusChanged(String provider, int status, Bundle extras) {}

    @Override
    public void onProviderEnabled(String provider) {}

    @Override
    public void onProviderDisabled(String provider) {}

    private void createLocationManagerIfNeeded(Context context) {
        if (mLocationManager != null) return;
        mLocationManager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
        if (mLocationManager == null) {
            Log.e(TAG, "Could not get location manager.");
        }
    }

    /**
     * Registers this object with the location service.
     */
    private void registerForLocationUpdates(Context context, boolean enableHighAccuracy) {
        createLocationManagerIfNeeded(context);
        if (usePassiveOneShotLocation()) return;

        assert !mIsRunning;
        mIsRunning = true;

        // We're running on the main thread. The C++ side is responsible to
        // bounce notifications to the Geolocation thread as they arrive in the mainLooper.
        try {
            Criteria criteria = new Criteria();
            if (enableHighAccuracy) criteria.setAccuracy(Criteria.ACCURACY_FINE);
            mLocationManager.requestLocationUpdates(0, 0, criteria, this, Looper.getMainLooper());
        } catch (SecurityException e) {
            Log.e(TAG,
                    "Caught security exception while registering for location updates "
                            + "from the system. The application does not have sufficient "
                            + "geolocation permissions.");
            unregisterFromLocationUpdates();
            // Propagate an error to JavaScript, this can happen in case of WebView
            // when the embedding app does not have sufficient permissions.

            try {
                mCallback.newErrorAvailable(
                        "application does not have sufficient geolocation permissions.");
            } catch (RemoteException e1) {
            }
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Caught IllegalArgumentException registering for location updates.");
            unregisterFromLocationUpdates();
            assert false;
        }
    }

    /**
     * Unregisters this object from the location service.
     */
    private void unregisterFromLocationUpdates() {
        if (!mIsRunning) return;
        mIsRunning = false;
        mLocationManager.removeUpdates(this);
    }

    private boolean usePassiveOneShotLocation() {
        if (!isOnlyPassiveLocationProviderEnabled()) {
            return false;
        }

        // Do not request a location update if the only available location provider is
        // the passive one. Make use of the last known location and call
        // onNewLocationAvailable directly.
        final Location location =
                mLocationManager.getLastKnownLocation(LocationManager.PASSIVE_PROVIDER);
        if (location != null) {
            // should called on main thread!!!
            try {
                mCallback.onNewLocationAvailable(location);
            } catch (RemoteException e) {
            }
        }
        return true;
    }

    /*
     * Checks if the passive location provider is the only provider available
     * in the system.
     */
    private boolean isOnlyPassiveLocationProviderEnabled() {
        final List<String> providers = mLocationManager.getProviders(true);
        return providers != null && providers.size() == 1
                && providers.get(0).equals(LocationManager.PASSIVE_PROVIDER);
    }
}
