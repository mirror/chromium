// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.ComponentName;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.process_launcher.ChildProcessCreationParams;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * This class is responsible for allocating and managing connections to child
 * process services. These connections are in a pool (the services are defined
 * in the AndroidManifest.xml).
 */
public class ChildConnectionAllocator {
    private static final String TAG = "ChildConnAllocator";

    /** Listener that clients can use to get notified when connections get freed. */
    public interface Listener {
        void onConnectionFreed(
                ChildConnectionAllocator allocator, ChildProcessConnection connection);
    }

    /** Factory interface. Used by tests to specialize created connections. */
    @VisibleForTesting
    protected interface ConnectionFactory {
        ChildProcessConnection createConnection(Context context,
                ChildProcessConnection.DeathCallback deathCallback, String serviceClassName,
                Bundle serviceBundle, ChildProcessCreationParams creationParams);
    }

    /** Default implementation of the ConnectionFactory that creates actual connections. */
    private static class ConnectionFactoryImpl implements ConnectionFactory {
        public ChildProcessConnection createConnection(Context context,
                ChildProcessConnection.DeathCallback deathCallback, String serviceClassName,
                Bundle serviceBundle, ChildProcessCreationParams creationParams) {
            return new ChildProcessConnection(
                    context, deathCallback, serviceClassName, serviceBundle, creationParams);
        }
    }

    // Connections to services. Indices of the array correspond to the service numbers.
    private final ChildProcessConnection[] mChildProcessConnections;

    private final String mServiceClassName;

    // The list of free (not bound) service indices.
    private final ArrayList<Integer> mFreeConnectionIndices;

    private final List<Listener> mListeners = new ArrayList<>();

    private ConnectionFactory mConnectionFactory = new ConnectionFactoryImpl();

    /**
     * Factory method that retrieves the service name and number of service from the
     * AndroidManifest.xml.
     */
    public static ChildConnectionAllocator create(Context context, String packageName,
            String serviceClassNameManifestKey, String numChildServicesManifestKey) {
        String serviceClassName = null;
        int numServices = -1;
        PackageManager packageManager = context.getPackageManager();
        try {
            ApplicationInfo appInfo =
                    packageManager.getApplicationInfo(packageName, PackageManager.GET_META_DATA);
            if (appInfo.metaData != null) {
                serviceClassName = appInfo.metaData.getString(serviceClassNameManifestKey);
                numServices = appInfo.metaData.getInt(numChildServicesManifestKey, -1);
            }
        } catch (PackageManager.NameNotFoundException e) {
            throw new RuntimeException("Could not get application info.");
        }

        if (numServices < 0) {
            throw new RuntimeException("Illegal meta data value for number of child services");
        }

        // Check that the service exists.
        try {
            // PackageManager#getServiceInfo() throws an exception if the service does not
            // exist.
            packageManager.getServiceInfo(
                    new ComponentName(packageName, serviceClassName + "0"), 0);
        } catch (PackageManager.NameNotFoundException e) {
            throw new RuntimeException("Illegal meta data value: the child service doesn't exist");
        }

        return new ChildConnectionAllocator(packageName, serviceClassName, numServices);
    }

    // TODO(jcivelli): remove this method once crbug.com/693484 has been addressed.
    static int getNumberOfServices(
            Context context, String packageName, String numChildServicesManifestKey) {
        int numServices = -1;
        try {
            PackageManager packageManager = context.getPackageManager();
            ApplicationInfo appInfo =
                    packageManager.getApplicationInfo(packageName, PackageManager.GET_META_DATA);
            if (appInfo.metaData != null) {
                numServices = appInfo.metaData.getInt(numChildServicesManifestKey, -1);
            }
        } catch (PackageManager.NameNotFoundException e) {
            throw new RuntimeException("Could not get application info", e);
        }
        if (numServices < 0) {
            throw new RuntimeException("Illegal meta data value for number of child services");
        }
        return numServices;
    }

    /**
     * Factory method used with some tests to create an allocator with values passed in directly
     * instead of being retrieved from the AndroidManifest.xml.
     */
    @VisibleForTesting
    public static ChildConnectionAllocator createForTest(
            String packageName, String serviceClassName, int serviceCount) {
        return new ChildConnectionAllocator(packageName, serviceClassName, serviceCount);
    }

    private ChildConnectionAllocator(
            String packageName, String serviceClassName, int numChildServices) {
        mServiceClassName = serviceClassName;
        mChildProcessConnections = new ChildProcessConnection[numChildServices];
        mFreeConnectionIndices = new ArrayList<Integer>(numChildServices);
        for (int i = 0; i < numChildServices; i++) {
            mFreeConnectionIndices.add(i);
        }
    }

    /** @return a connection ready to be bound, or null if there are no free slots. */
    public ChildProcessConnection allocate(Context context, Bundle serviceBundle,
            ChildProcessCreationParams creationParams,
            ChildProcessConnection.DeathCallback deathCallback) {
        assert LauncherThread.runningOnLauncherThread();
        if (mFreeConnectionIndices.isEmpty()) {
            Log.d(TAG, "Ran out of services to allocate.");
            return null;
        }
        int slot = mFreeConnectionIndices.remove(0);
        assert mChildProcessConnections[slot] == null;
        String serviceClassName = mServiceClassName + slot;
        mChildProcessConnections[slot] = mConnectionFactory.createConnection(
                context, deathCallback, serviceClassName, serviceBundle, creationParams);
        Log.d(TAG, "Allocator allocated a connection, name: %s, slot: %d", mServiceClassName, slot);
        return mChildProcessConnections[slot];
    }

    /** Frees a connection and notifies listeners. */
    public void free(ChildProcessConnection connection) {
        assert LauncherThread.runningOnLauncherThread();
        // mChildProcessConnections is relatively short (20 items at max at this point).
        // We are better of iterating than caching in a map.
        int slot = Arrays.asList(mChildProcessConnections).indexOf(connection);
        if (slot == -1) {
            Log.e(TAG, "Unable to find connection to free.");
            assert false;
        } else {
            mChildProcessConnections[slot] = null;
            assert !mFreeConnectionIndices.contains(slot);
            mFreeConnectionIndices.add(slot);
            Log.d(TAG, "Allocator freed a connection, name: %s, slot: %d", mServiceClassName, slot);
        }
        // Copy the listeners list so listeners can unregister themselves while we are iterating.
        List<Listener> listeners = new ArrayList<>(mListeners);
        for (Listener listener : listeners) {
            listener.onConnectionFreed(this, connection);
        }
    }

    public boolean isFreeConnectionAvailable() {
        assert LauncherThread.runningOnLauncherThread();
        return !mFreeConnectionIndices.isEmpty();
    }

    public int getNumberOfServices() {
        return mChildProcessConnections.length;
    }

    public void addListener(Listener listener) {
        mListeners.add(listener);
    }

    public void removeListener(Listener listener) {
        mListeners.remove(listener);
    }

    @VisibleForTesting
    void setConnectionFactoryForTesting(ConnectionFactory connectionFactory) {
        mConnectionFactory = connectionFactory;
    }

    /** @return the count of connections managed by the allocator */
    @VisibleForTesting
    int allocatedConnectionsCountForTesting() {
        assert LauncherThread.runningOnLauncherThread();
        return mChildProcessConnections.length - mFreeConnectionIndices.size();
    }

    @VisibleForTesting
    ChildProcessConnection[] connectionArrayForTesting() {
        return mChildProcessConnections;
    }
}
