// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.edk;

import android.os.IBinder;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.util.List;

/**
 * This class is used to specialize MultiprocessTestClientServiceDelegate behavior for mojo, so it
 * sets up its parcelable channels that are used to exchange parcelables accross processes.
 */
@JNINamespace("mojo::edk")
public class MultiprocessTestServiceHelper {
    private static List<IBinder> sCallbacks;

    /**
     * Called when the MultiProcessTestService is bound, through reflection.
     * @return true if the initialization was successful.
     */
    public static void initializeMultiprocessTestService(List<IBinder> callbacks) {
        assert sCallbacks == null;
        sCallbacks = callbacks;
    }

    @CalledByNative
    private static IBinder[] getParcelableChannelIBinders() {
        return sCallbacks.toArray(new IBinder[0]);
    }

    // Prevent instantiation.
    private MultiprocessTestServiceHelper() {}
}