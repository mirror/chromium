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

/**
 * Implementation of MultiprocessTestClientLauncher.Delegate used to pass the parcelable channel
 * related interfaces to the started process.
 */
@JNINamespace("mojo::edk")
public class MultiprocessTestLauncherDelegate implements MultiprocessTestClientLauncher.Delegate {
    private final IBinder[] mClientInterfaces;

    @Override
    public List<IBinder> getExtraClientInterfaces() {
        return mClientInterfaces == null ? null : Arrays.asList(mClientInterfaces);
    }

    @Override
    public String getInitClass() {
        return MultiprocessTestServiceHelper.class.getName();
    }

    private MultiprocessTestLauncherDelegate(IBinder[] clientInterfaces) {
        mClientInterfaces = clientInterfaces;
    }

    @CalledByNative
    private static MultiprocessTestLauncherDelegate create(IBinder[] clientInterfaces) {
        return new MultiprocessTestLauncherDelegate(clientInterfaces);
    }
}
