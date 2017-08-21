// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.edk;

import android.os.IBinder;
import android.os.RemoteException;

import org.chromium.base.Log;

import java.util.List;

import javax.annotation.concurrent.GuardedBy;

/**
 * This class is used to specialize MultiprocessTestClientServiceDelegate behavior for mojo, so it
 * sets up its parcelable channels that are used to exchange parcelables accross processes.
 */
public class MultiprocessTestServiceHelper {
    private static final String TAG = "MulProcTestSrvHelper";

    private static final Object sInstanceLock = new Object();
    @GuardedBy("sInstanceLock")
    private static MultiprocessTestServiceHelper sInstance;

    private final ParcelableChannelServer mParcelableChannelServer = new ParcelableChannelServer();
    private final ParcelableChannelClient mParcelableChannelClient;

    private MultiprocessTestServiceHelper(IParcelableChannel iparcelableChannelClient) {
        mParcelableChannelClient = new ParcelableChannelClient(iparcelableChannelClient);
    }

    /**
     * Called when the MultiProcessTestService is bound, through reflection.
     * @return true if the initialization was successful.
     */
    public static boolean initializeMultiprocessTestService(List<IBinder> callbacks) {
        synchronized (sInstanceLock) {
            if (sInstance != null) return false;
            assert callbacks.size() >= 2;
            // 1st IBinder is the IParcelableChannel to send Parcelables to the client.
            sInstance = new MultiprocessTestServiceHelper(
                    IParcelableChannel.Stub.asInterface(callbacks.get(0)));
            // 2nd IBinder is the IParcelableChannelProvider that we use to pass our
            // IParcelableChannel to the client so we can receive Parcelable from it.
            IParcelableChannelProvider parcelableChannelProvider =
                    IParcelableChannelProvider.Stub.asInterface(callbacks.get(1));
            try {
                parcelableChannelProvider.sendParcelableChannel(sInstance.mParcelableChannelServer);
            } catch (RemoteException re) {
                Log.e(TAG, "Failed to pass parcelable channel to client", re);
            }
            return true;
        }
    }
}