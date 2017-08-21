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
 * Helper class used by native code to access the ParcelableChannels used to send Parcelable objects
 * to child processes.
 */
@JNINamespace("mojo::edk")
public class MultiprocessTestLauncherDelegate implements MultiprocessTestClientLauncher.Delegate {
    private final IBinder[] mCallbacks;

    @Override
    public List<IBinder> getExtraCallbacks() {
        return Arrays.asList(mCallbacks);
    }

    @Override
    public String getInitClass() {
        return MultiprocessTestServiceHelper.class.getName();
    }

    private MultiprocessTestLauncherDelegate(IBinder[] callbacks) {
        mCallbacks = callbacks;
    }

    @CalledByNative
    private static MultiprocessTestLauncherDelegate create(IBinder[] callbacks) {
        return new MultiprocessTestLauncherDelegate(callbacks);
    }
}