// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.edk;

import android.os.IBinder;
import android.os.Parcelable;
import android.os.RemoteException;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Provides an implementation of IParcelableChannel that stores the received parcelables and make
 * them accessible to native code.
 */
@JNINamespace("mojo::edk")
public class ParcelableChannelServer extends IParcelableChannel.Stub {
    private static final String TAG = "ParcelableChanServer";

    private long mNativeParcelableChannelServer;

    private ParcelableChannelServer(long nativeParcelableChannelServer) {
        mNativeParcelableChannelServer = nativeParcelableChannelServer;
    }

    @Override
    public void sendParcelable(ParcelableWrapper parcelableValue) {
        if (mNativeParcelableChannelServer != 0) {
            nativeOnParcelableReceived(mNativeParcelableChannelServer, parcelableValue.getId(),
                    parcelableValue.getParcelable());
        }
    }

    @CalledByNative
    private static ParcelableChannelServer create(long nativeParcelableChannelServer) {
        return new ParcelableChannelServer(nativeParcelableChannelServer);
    }

    @CalledByNative
    private void invalidate() {
        mNativeParcelableChannelServer = 0;
    }

    @CalledByNative
    private boolean setChannelOnParent(IBinder parcelableChannelProvider) {
        try {
            IParcelableChannelProvider.Stub.asInterface(parcelableChannelProvider)
                    .sendParcelableChannel(this);
            return true;
        } catch (RemoteException re) {
            Log.e(TAG, "Failed to set ParcelableChannel on parent.", re);
            return false;
        }
    }

    private native void nativeOnParcelableReceived(
            long nativeParcelableChannelServer, int id, Parcelable parcelable);
}
