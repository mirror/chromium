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
 * Wraps a IParcelableChannel so it can be called from native code.
 */
@JNINamespace("mojo::edk")
public final class ParcelableChannelClient {
    private static final String TAG = "ParcelChanClient";

    private IParcelableChannel mIParcelableChannel;

    private ParcelableChannelClient(IParcelableChannel parcelableChannel) {
        mIParcelableChannel = parcelableChannel;
    }

    @CalledByNative
    public static ParcelableChannelClient create(IBinder parcelableChannel) {
        return new ParcelableChannelClient(IParcelableChannel.Stub.asInterface(parcelableChannel));
    }

    @CalledByNative
    public boolean sendParcelable(int id, Parcelable parcelable) {
        try {
            mIParcelableChannel.sendParcelable(new ParcelableWrapper(id, parcelable));
            return true;
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to send parcelable.", e);
            return false;
        }
    }
}