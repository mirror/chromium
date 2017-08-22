// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.edk;

import android.os.IBinder;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.util.List;

/**
 * This class is used to specialize MultiprocessTestClientServiceDelegate behavior for mojo tests.
 * TODO(jcivelli): make the native code retrieve the client interfaces to set up the parcelable
 * channels.
 */
@JNINamespace("mojo::edk")
public class MultiprocessTestServiceHelper {
    private static List<IBinder> sClientInterfaces;

    /** Called when the MultiProcessTestService is bound, through reflection. */
    public static void initializeMultiprocessTestService(List<IBinder> clientInterfaces) {
        assert sClientInterfaces == null;
        sClientInterfaces = clientInterfaces;
    }

    @CalledByNative
    private static IBinder[] getParcelableChannelIBinders() {
        return sClientInterfaces.toArray(new IBinder[0]);
    }

    // Prevent instantiation.
    private MultiprocessTestServiceHelper() {}
}