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

import java.util.ArrayList;
import java.util.List;

import javax.annotation.concurrent.GuardedBy;

/**
 * Allows to send parcelable over an IParcelableChannel that is not initially available and
 * retrieved through an IParcelableChannelProvider. Parcelables sent before the actual channel is
 * set are buffered and sent when the channel becomes available.
 */
@JNINamespace("mojo::edk")
public final class ParcelableChannelClientProxy {
    private static final String TAG = "ParcelChanClientPrxy";

    private final IParcelableChannelProvider.Stub mParcelableChannelProvider =
            new IParcelableChannelProvider.Stub() {
                @Override
                public void sendParcelableChannel(IParcelableChannel channel) {
                    // This is called on a background thread.
                    synchronized (mLock) {
                        mIParcelableChannel = channel;
                        flushParcelableQueue();
                    }
                }
            };

    private final Object mLock = new Object();

    @GuardedBy("mLock")
    private IParcelableChannel mIParcelableChannel;

    // The list of parcelables that have not been sent yet because the channel was not yet
    // available.
    @GuardedBy("mLock")
    private final List<ParcelableWrapper> mPendingParcelables = new ArrayList<>();

    private ParcelableChannelClientProxy() {}

    @GuardedBy("mLock")
    private void flushParcelableQueue() {
        for (ParcelableWrapper parcelable : mPendingParcelables) {
            sendNow(parcelable);
        }
        mPendingParcelables.clear();
    }

    @GuardedBy("mLock")
    private boolean sendNow(ParcelableWrapper parcelable) {
        assert mIParcelableChannel != null;
        try {
            mIParcelableChannel.sendParcelable(parcelable);
            return true;
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to send parcelable.", e);
            return false;
        }
    }

    @CalledByNative
    public static ParcelableChannelClientProxy create() {
        return new ParcelableChannelClientProxy();
    }

    @CalledByNative
    public IBinder getIParcelableChannelProvider() {
        return mParcelableChannelProvider;
    }

    @CalledByNative
    public boolean sendParcelable(int id, Parcelable parcelable) {
        ParcelableWrapper parcelableToSend = new ParcelableWrapper(id, parcelable);
        synchronized (mLock) {
            if (mIParcelableChannel != null) {
                return sendNow(parcelableToSend);
            }
            mPendingParcelables.add(parcelableToSend);
            return true;
        }
    }
}