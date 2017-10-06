// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

/**
 * A base class for all Callback<T> that are owned by Java.
 * @param <T> The type of the parameter for {@link Callback#onResult(Object)}.
 */
public abstract class Callback1<T> implements Callback<T> {
    final NativeRefHolder mRefHolder;

    // TOOD(nyquist): Auto-generate code to construct the different types of callbacks.
    Callback1(long nativePtr) {
        mRefHolder = NativeRefAssassin.getInstance().registerReference(
                nativePtr, this, Callback1::nativeDestroy);
    }

    private static native void nativeDestroy(long nativePtr);
}
