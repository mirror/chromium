// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.edk;

import android.os.Parcelable;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.util.HashMap;
import java.util.Map;

import javax.annotation.concurrent.GuardedBy;

/**
 * Provides an implementation of IParcelableChannel that stores the received parcelables and make
 * them accessible to native code.
 */
@JNINamespace("mojo::edk")
public class ParcelableChannelServer extends IParcelableChannel.Stub {
    private static final String TAG = "ParcelableChanServer";

    private final Object mLock = new Object();

    @GuardedBy("mLock")
    private final Map<Integer, Parcelable> mReceivedParcelables = new HashMap<>();

    public void clearReceivedParcelables() {
        synchronized (mLock) {
            mReceivedParcelables.clear();
        }
    }

    @Override
    public void sendParcelable(ParcelableWrapper parcelableValue) {
        synchronized (mLock) {
            mReceivedParcelables.put(parcelableValue.getId(), parcelableValue.getParcelable());
            mLock.notifyAll();
        }
    }

    /** Returns the Parcelable associated with id, potentially blocking until there is one. */
    @CalledByNative
    private Parcelable takeParcelable(int id) {
        Parcelable parcelable = null;
        synchronized (mLock) {
            while (parcelable == null) {
                parcelable = mReceivedParcelables.remove(id);
                if (parcelable == null) {
                    try {
                        mLock.wait();
                    } catch (InterruptedException ie) {
                        Log.e(TAG, "Interrupted while waiting.", ie);
                    }
                }
            }
        }
        return parcelable;
    }
}