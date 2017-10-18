// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.os.Handler;
import android.os.Looper;
import android.support.annotation.NonNull;

import java.lang.ref.PhantomReference;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.util.HashSet;
import java.util.Set;

import javax.annotation.concurrent.GuardedBy;
import javax.annotation.concurrent.ThreadSafe;

/**
 * NativeRefAssassin enables callers to register Java objects that use {@link NativeRefHolder},
 * and when the objects are garbage collected, the {@link Destroyer} is invoked with the pointer
 * they registered with. This makes it possible to automatically cleaning up native objects that
 * are not destroyed in a manual way from Java.
 *
 * To get the singleton instance of this object, invoke {@link NativeRefAssassin#getInstance()}.
 * To register an object with this class, use {@link NativeRefAssassin#registerReference(long,
 * Object, Destroyer, Handler)}.
 *
 * A typical class using this would be:
 *   class MyClass {
 *
 *     @CalledByNative
 *     private static MyClass create(long nativePtr) {
 *       return new MyClass(nativePtr);
 *     }
 *
 *     private final NativeRefHolder mRefHolder;
 *
 *     private MyClass(long nativePtr) {
 *       mRefHolder = NativeRefAssassin.getInstance().registerReference(
 *       nativePtr, this, MyClass::nativeDestroy);
 *     }
 *
 *     // Optional. Used if eager destruction is wanted.
 *     public void destroy() {
 *       mRefHolder.destroy();
 *     }
 *
 *     private static native nativeDestroy(long nativePtr);
 *   }
 *
 * IMPORTANT: The {@link Destroyer} you pass in must not have a reference back to the object that
 * should be garbage collected. In practice this means that the method reference should be static.
 */
@ThreadSafe
public class NativeRefAssassin {
    private static final String TAG = "NativeRefAssassin";

    /**
     * Holds on to all garbage collected objects.
     *
     * This class is thread safe for enqueuing and removing.
     */
    private final ReferenceQueue<Object> mReferenceQueue = new ReferenceQueue<>();

    private final Object mStrongReferencesLock = new Object();
    /**
     * The NativePhantomReference objects themselves must not be garbage collected, so they are
     * kept around until their underlying object is garbage collected.
     */
    @GuardedBy("mStrongReferencesLock")
    private final Set<NativePhantomReference> mStrongReferences = new HashSet<>();

    /**
     * The handler to post destroys to. In prod code, this will be a {@link Handler} for the
     * main thread.
     */
    private final Handler mMainThreadHandler;

    /**
     * The {@link Runnable} that constantly monitors references in the {@link ReferenceQueue}.
     */
    private final CleanerRunnable mCleanerRunnable = new CleanerRunnable();

    private final Object mCleanerThreadLock = new Object();

    /**
     * The thread used for running the {@link CleanerRunnable}.
     */
    @GuardedBy("mCleanerThreadLock")
    private Thread mCleanerThread;

    /**
     * Invoked when an object has been garbage collected.
     *
     * A typical implementation of this would be a your own static nativeDestroy method:
     * private static native nativeDestroy(long nativePtr);
     * You can refer to that with MyClass::nativeDestroy.
     *
     * IMPORTANT: The {@link NativeRefAssassin.Destroyer} must not have a reference back to the
     * object that should be garbage collected. In practice this means that the method reference
     * should be static.
     *
     * @see NativeRefHolder
     * @see NativeRefAssassin#registerReference(long, Object, Destroyer, Handler)
     *
     * @param <T> The type of Java object this supports destroying.
     */
    public interface Destroyer<T> { void onDestroy(long nativePtr); }

    private static class LazyHolder {
        private static final NativeRefAssassin INSTANCE = new NativeRefAssassin();
    }

    /**
     * @return the singleton instance of this class.
     */
    public static NativeRefAssassin getInstance() {
        return LazyHolder.INSTANCE;
    }

    /**
     * The NativePhantomReference is the core reference class for the {@link NativeRefAssassin}. Its
     * {@code referent} is the object that is being watched for garbage collection, and the
     * {@link NativeRefHolder} is the object that holds on to the native pointer and has
     * functionality for invoking destruction of native objects.
     */
    private static class NativePhantomReference extends PhantomReference<Object> {
        private final NativeRefHolder mNativeRefHolder;

        private NativePhantomReference(Object referent, ReferenceQueue<Object> referenceQueue,
                NativeRefHolder nativeRefHolder) {
            super(referent, referenceQueue);
            mNativeRefHolder = nativeRefHolder;
        }
    }

    /**
     * The CleanerRunnable is the code that is constantly watching for garbage collection of
     * registered objects. As soon as an object is collected, it posts a message to the
     * destroying handler (main thread in prod code) telling it to destruct the object.
     */
    private class CleanerRunnable implements Runnable {
        private volatile boolean mRunning = true;

        @Override
        public void run() {
            while (mRunning) {
                try {
                    Reference<? extends Object> ref = mReferenceQueue.remove();
                    clean((NativePhantomReference) ref);
                } catch (InterruptedException e) {
                    Log.e(TAG, "InterruptedException while waiting for NativePhantomReference");
                }
            }
        }

        private void stop() {
            mRunning = false;
        }

        private void clean(NativePhantomReference nativePhantomReference) {
            // Destroy is guaranteed to post the destruction to its destroy handler.
            nativePhantomReference.mNativeRefHolder.destroy();

            // It is required to clear out the referent of the PhantomReference, and also remove
            // the strong reference to the PhantomReference.
            nativePhantomReference.clear();
            synchronized (mStrongReferencesLock) {
                mStrongReferences.remove(nativePhantomReference);
            }
        }
    }

    @VisibleForTesting
    NativeRefAssassin() {
        mMainThreadHandler = new Handler(Looper.getMainLooper());
    }

    /**
     * Registers the {@code obj} to watch for garbage collection. Invokes the {@link Destroyer} when
     * {@code obj} is garbage collected, on the handler given in {@code destroyingHandler}.
     *
     * IMPORTANT: The {@link Destroyer} you pass in must not have a reference back to the object
     * that should be garbage collected. In practice this means that you will pass in a method
     * reference to a static member.
     *
     * @param nativePtr         the pointer to the native object.
     * @param obj               the object to watch out for garbage collection of.
     * @param destroyer         what to invoke when an object should be destroyed.
     * @param destroyingHandler the {@link Handler} to invoke the {@link Destroyer} on.
     * @return the {@link NativeRefHolder} that the object should hold on to.
     */
    public <T> NativeRefHolder registerReference(long nativePtr, @NonNull T obj,
            @NonNull Destroyer<T> destroyer, Handler destroyingHandler) {
        startIfNecessary();

        NativeRefHolder<T> nativeRefHolder =
                new NativeRefHolder<>(nativePtr, destroyer, destroyingHandler);
        NativePhantomReference nativePhantomReference =
                new NativePhantomReference(obj, mReferenceQueue, nativeRefHolder);
        synchronized (mStrongReferencesLock) {
            mStrongReferences.add(nativePhantomReference);
        }
        return nativeRefHolder;
    }

    /**
     * Registers the {@code obj} to watch for garbage collection. Invokes the {@link Destroyer} when
     * {@code obj} is garbage collected, on the main handler.
     *
     * IMPORTANT: The {@link Destroyer} you pass in must not have a reference back to the object
     * that should be garbage collected. In practice this means that you will pass in a method
     * reference to a static member.
     *
     * @param nativePtr the pointer to the native object.
     * @param obj       the object to watch out for garbage collection of.
     * @param destroyer         what to invoke when an object should be destroyed.
     * @return the {@link NativeRefHolder} that the object should hold on to.
     */
    public <T> NativeRefHolder registerReferenceMainHandlerDestruction(
            long nativePtr, @NonNull T obj, @NonNull Destroyer<T> destroyer) {
        return registerReference(nativePtr, obj, destroyer, mMainThreadHandler);
    }

    private void startIfNecessary() {
        synchronized (mCleanerThreadLock) {
            if (mCleanerThread != null) return;

            mCleanerThread = new Thread(new CleanerRunnable(), "NativeRefAssassin");
            mCleanerThread.start();
        }
    }

    @VisibleForTesting
    void stopForTest() {
        synchronized (mCleanerThreadLock) {
            mCleanerRunnable.stop();
            if (mCleanerThread != null) mCleanerThread.interrupt();
            mCleanerThread = null;
        }
    }
}
