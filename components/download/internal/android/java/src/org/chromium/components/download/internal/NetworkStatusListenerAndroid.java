// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.download.internal;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.net.NetworkChangeNotifierAutoDetect;
import org.chromium.net.NetworkChangeNotifierAutoDetect.Observer;
import org.chromium.net.RegistrationPolicyAlwaysRegister;

/**
 * Network listener that can notify network connectivity changes to native side
 * download::NetworkStatusListenerAndroid even when the Android application is in the background.
 *
 * The class is created and owned by native side, and lives and propagate network change events
 * on the UI thread.
 */
@JNINamespace("download")
public final class NetworkStatusListenerAndroid implements Observer {
    private long mNativePtr;
    private NetworkChangeNotifierAutoDetect mNotifier;

    private void registerNotifier() {
        if (mNotifier != null) return;
        // Register policy that can fire network change events when the application is in the
        // background.
        mNotifier =
                new NetworkChangeNotifierAutoDetect(this, new RegistrationPolicyAlwaysRegister());
    }

    @Override
    public void onConnectionTypeChanged(int newConnectionType) {
        if (mNativePtr != 0) {
            nativeNotifyNetworkChange(mNativePtr, newConnectionType);
        }
    }

    @CalledByNative
    public int getCurrentConnectionType() {
        assert mNotifier != null;
        return mNotifier.getCurrentNetworkState().getConnectionType();
    }

    private NetworkStatusListenerAndroid(long nativePtr) {
        mNativePtr = nativePtr;
        registerNotifier();
    }

    @CalledByNative
    private static NetworkStatusListenerAndroid create(long nativePtr) {
        return new NetworkStatusListenerAndroid(nativePtr);
    }

    @CalledByNative
    private void clearNativePtr() {
        mNativePtr = 0;
    }

    @Override
    public void onConnectionSubtypeChanged(int newConnectionSubtype) {}
    @Override
    public void onNetworkConnect(long netId, int connectionType) {}
    @Override
    public void onNetworkSoonToDisconnect(long netId) {}
    @Override
    public void onNetworkDisconnect(long netId) {}
    @Override
    public void purgeActiveNetworkList(long[] activeNetIds) {}
    private native void nativeNotifyNetworkChange(
            long nativeNetworkStatusListenerAndroid, int connectionType);
}
