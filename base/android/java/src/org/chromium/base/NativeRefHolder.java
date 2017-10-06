// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import java.util.concurrent.atomic.AtomicLong;

/**
 * A class that holds on to a native pointer that supports being automatically destroyed by a
 * {@link NativeRefAssassin}.
 *
 * Owners of this can always eagerly invoke {@link #destroy()} if they want to destroy the native
 * object early. It is NOT OK to invoke {@link NativeRefAssassin.Destroyer#onDestroy(long)}
 * directly, as it will not update the state for whether the native object has already been
 * destroyed or not.
 *
 * The {@link NativeRefAssassin.Destroyer#onDestroy(long)} will always be invoked on the main
 * thread (in prod environments).
 *
 * A typical usage pattern for NativeRefHolder is by adding something like this in your
 * constructor:
 * mRefHolder = NativeRefAssassin.getInstance().registerReference(
 *     nativePtr, this, MyClass::nativeDestroy);
 *
 * To access your native pointer, you would then use mRefHolder.getNativePtr(), which will be 0
 * if the object has already been destroyed.
 *
 * IMPORTANT: The {@link NativeRefAssassin.Destroyer} you pass in must not have a reference back to
 * the object that should be garbage collected. In practice this means that the method reference
 * should be static.
 */
public class NativeRefHolder {
    private final AtomicLong mNativePtr;
    private final NativeRefAssassin.Destroyer mDestroyer;

    /**
     * @param nativePtr the pointer to the native object that this NativeRefHolder owns.
     * @param destroyer what to invoke when {@link #destroy()} is invoked.
     */
    NativeRefHolder(long nativePtr, NativeRefAssassin.Destroyer destroyer) {
        mNativePtr = new AtomicLong(nativePtr);
        mDestroyer = destroyer;
    }

    /**
     * @return the native pointer, or 0 if it has already been destroyed.
     */
    public final long getNativePtr() {
        return mNativePtr.get();
    }

    /**
     * Destroys the native pointer and ensures that it is not possible to retrieve the native
     * pointer anymore. It is only possible to destroy the object once, so invoking this method
     * multiple times is fine; only the first call to
     * {@link NativeRefAssassin.Destroyer#onDestroy(long)}
     * will happen
     * in that case.
     */
    public final void destroy() {
        long ptr = mNativePtr.getAndSet(0);
        if (ptr != 0) mDestroyer.onDestroy(ptr);
    }
}
