// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.edk;

import android.os.IBinder;

import org.chromium.base.MultiprocessTestClientLauncher;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.util.Arrays;
import java.util.List;

import javax.annotation.concurrent.GuardedBy;

/**
 * Helper class used by native code to access the ParcelableChannels used to send Parcelable objects
 * to child processes.
 */

@JNINamespace("mojo::edk")
public class MultiprocessTestLauncherHelper {
    private final ParcelableChannelServer mParcelableChannelServer = new ParcelableChannelServer();

    private final IParcelableChannelProvider.Stub mParcelableChannelProvider =
            new IParcelableChannelProvider.Stub() {
                @Override
                public void sendParcelableChannel(IParcelableChannel channel) {
                    synchronized (mParcelableChannelClientLock) {
                        assert mParcelableChannelClient == null;
                        mParcelableChannelClient = new ParcelableChannelClient(channel);
                        mParcelableChannelClientLock.notifyAll();
                    }
                }
            };

    private final MultiprocessTestClientLauncher.Delegate mLauncherDelegate =
            new MultiprocessTestClientLauncher.Delegate() {
                @Override
                public List<IBinder> getExtraCallbacks() {
                    // Send the channel the child process will use to send Parcelable, as well the
                    // binder the child process will send its channel on (the one we'll use to send
                    // parcelable to it).
                    return Arrays.asList(
                            (IBinder) mParcelableChannelServer, mParcelableChannelProvider);
                }

                @Override
                public String getInitClass() {
                    return MultiprocessTestServiceHelper.class.getName();
                }
            };

    private final Object mParcelableChannelClientLock = new Object();
    @GuardedBy("mParcelableChannelClientLock")
    private ParcelableChannelClient mParcelableChannelClient;

    @CalledByNative
    private MultiprocessTestClientLauncher.Delegate getDelegate() {
        return mLauncherDelegate;
    }

    @CalledByNative
    private ParcelableChannelServer getParcelableChannelServer() {
        return mParcelableChannelServer;
    }

    @CalledByNative
    private ParcelableChannelClient getParcelableChannelClientBlocking() {
        synchronized (mParcelableChannelClientLock) {
            while (mParcelableChannelClient == null) {
                try {
                    mParcelableChannelClientLock.wait();
                } catch (InterruptedException ie) {
                    // Ignore.
                }
            }
        }
        return mParcelableChannelClient;
    }

    @CalledByNative
    private static MultiprocessTestLauncherHelper create() {
        return new MultiprocessTestLauncherHelper();
    }
}