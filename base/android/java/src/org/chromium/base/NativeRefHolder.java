// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.os.Handler;

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
 * The {@link NativeRefAssassin.Destroyer#onDestroy(long)} will be invoked on the
 * {@code destroyHandler} {@link Handler} passed in to
 * {@link NativeRefAssassin#registerReference(long, Object, NativeRefAssassin.Destroyer, Handler)},
 * or in on the main handler in the case of a call to {@link
 * NativeRefAssassin#registerReferenceMainHandlerDestruction(long, Object,
 * NativeRefAssassin.Destroyer)}.
 *
 * A typical usage pattern for NativeRefHolder is by adding something like this in your
 * constructor:
 * mRefHolder = NativeRefAssassin.getInstance().registerReferenceMainHandlerDestruction(
 *     nativePtr, this, MyClass::nativeDestroy);
 *
 * To access your native pointer, you would then use {@code mRefHolder.getNativePtr()}, which will
 * be 0 if the object has already been destroyed or is in the process of being destroyed.
 *
 * IMPORTANT: The {@link NativeRefAssassin.Destroyer} you pass in must not have a reference back to
 * the object that should be garbage collected. In practice this means that the method reference
 * should be static.
 *
 * @param <T> The type of Java object this supports destroying.
 */
public class NativeRefHolder<T> {
    private final AtomicLong mNativePtr;
    private final NativeRefAssassin.Destroyer<T> mDestroyer;
    private final Handler mDestroyHandler;

    /**
     * @param nativePtr the pointer to the native object that this NativeRefHolder owns.
     * @param destroyer what to invoke when an object is should be destroyed.
     * @param destroyHandler the {@link Handler} to invoke the {@link NativeRefAssassin.Destroyer}
     * on.
     */
    NativeRefHolder(
            long nativePtr, NativeRefAssassin.Destroyer<T> destroyer, Handler destroyHandler) {
        mNativePtr = new AtomicLong(nativePtr);
        mDestroyer = destroyer;
        mDestroyHandler = destroyHandler;
    }

    /**
     * When using the value returned from this method, you must ensure that the native object
     * will not be destroyed while you are using it. One way to ensure that it's not destroyed
     * within the stack frame as the call to {@link #getNativePtr()} is to ensure that you are
     * using it on the same {@link Handler} as this {@link NativeRefHolder} was registered with
     * on the {@link NativeRefAssassin}. Any calls to {@link #destroy()} will be posted to that
     * {@link Handler}.
     *
     * @return the native pointer, or 0 if it has already been destroyed.
     */
    public final long getNativePtr() {
        return mNativePtr.get();
    }

    /**
     * Clears the native pointer and posts a task to the destroy {@link Handler} to destroy
     * the native object. It is guaranteed that it is only possible to destroy the object once, so
     * invoking this method multiple times is fine; only the first call to
     * {@link NativeRefAssassin.Destroyer#onDestroy(long)}
     * will be posted in that case.
     *
     * This method may be invoked from any thread.
     */
    public final void destroy() {
        long ptr = mNativePtr.getAndSet(0);
        if (ptr == 0) return;

        mDestroyHandler.post(() -> mDestroyer.onDestroy(ptr));
    }
}
