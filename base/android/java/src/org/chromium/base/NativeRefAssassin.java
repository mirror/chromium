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
 * are
 * not destroyed in a manual way in Java.
 *
 * To get an instance of this object, invoke {@link NativeRefAssassin#getInstance()}.
 * To register an object with this class, use {@link NativeRefAssassin#registerReference(long,
 * Object, Destroyer)}.
 *
 * A typical class using this would be:
 * class MyClass {
 *   @CalledByNative
 *   private static MyClass create(long nativePtr) {
 *     return new MyClass(nativePtr);
 *   }
 *
 *   private final NativeRefHolder mRefHolder;
 *
 *   private MyClass(long nativePtr) {
 *     mRefHolder = NativeRefAssassin.getInstance().registerReference(
 *         nativePtr, this, MyClass::nativeDestroy);
 *   }
 *
 *   // Optional. Used if eager destruction is wanted.
 *   public void destroy() {
 *     mRefHolder.destroy();
 *   }
 *
 *   private static native nativeDestroy(long nativePtr);
 * }
 *
 * IMPORTANT: The {@link Destroyer} you pass in must not have a reference back to the object that
 * should be garbage collected. In practice this means that the method reference should be static.
 */
@ThreadSafe
public class NativeRefAssassin {
    private static final String TAG = "NativeRefAssassin";

    // This class is thread safe for enqueuing and removing.
    private final ReferenceQueue<Object> mReferenceQueue = new ReferenceQueue<>();

    // The NativePhantomReference objects themselves must not be garbage collected, so they are
    // kept around until their underlying object is garbage collected.
    @GuardedBy("mStrongReferencesLock")
    private final Set<NativePhantomReference> mStrongReferences = new HashSet<>();
    private final Object mStrongReferencesLock = new Object();

    /**
     * The handler to post destroys to. In prod code, this will be a {@link Handler} for the
     * main thread.
     */
    private final Handler mDestroyingHandler;

    /**
     * The {@link Runnable} that constantly monitors references in the {@link ReferenceQueue}.
     */
    private final CleanerRunnable mCleanerRunnable = new CleanerRunnable();

    @GuardedBy("mThreadLock")
    private final Object mThreadLock = new Object();
    private Thread mThread;

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
     * @see NativeRefAssassin#registerReference(long, Object, Destroyer)
     */
    public interface Destroyer { void onDestroy(long nativePtr); }

    private static class LazyHolder {
        private static final NativeRefAssassin INSTANCE =
                new NativeRefAssassin(new Handler(Looper.getMainLooper()));
    }

    private static NativeRefAssassin sInstanceForTest;

    @VisibleForTesting
    static void setInstanceForTest(NativeRefAssassin nativeRefAssassin) {
        sInstanceForTest = nativeRefAssassin;
    }

    /**
     * @return the singleton instance of this class.
     */
    public static NativeRefAssassin getInstance() {
        if (sInstanceForTest != null) return sInstanceForTest;

        return LazyHolder.INSTANCE;
    }

    /**
     * The NativePhantomReference is the core reference class for the {@link NativeRefAssassin}. Its
     * |referent| is the object that is being watched for garbage collection, and the
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
                    final Reference<? extends Object> ref = mReferenceQueue.remove();
                    if (ref instanceof NativePhantomReference) {
                        clean((NativePhantomReference) ref);
                    }
                } catch (InterruptedException e) {
                    Log.w(TAG, "InterruptedException while waiting for NativePhantomReference");
                }
            }
        }

        private void stop() {
            mRunning = false;
        }

        private void clean(NativePhantomReference nativePhantomReference) {
            final NativeRefHolder nativeRefHolder = nativePhantomReference.mNativeRefHolder;
            mDestroyingHandler.post(nativeRefHolder::destroy);

            // It is required to clear out the referent of the PhantomReference, and also remove
            // the strong reference to the PhantomReference.
            nativePhantomReference.clear();
            synchronized (mStrongReferencesLock) {
                mStrongReferences.remove(nativePhantomReference);
            }
        }
    }

    @VisibleForTesting
    NativeRefAssassin(Handler destroyingHandler) {
        mDestroyingHandler = destroyingHandler;
    }

    /**
     * Registers the |obj| to watch for garbage collection. Invokes the destroyer when |obj| is
     * garbage collected.
     *
     * IMPORTANT: The {@link Destroyer} you pass in must not have a reference back to the object
     * that should be garbage collected. In practice this means that you will pass in a method
     * reference to a static member.
     *
     * @param nativePtr the pointer to the native object.
     * @param obj       the object to watch out for garbage collection of.
     * @param destroyer what to invoke when an object is destroyed.
     * @return the {@link NativeRefHolder} that the object should hold on to.
     */
    public NativeRefHolder registerReference(
            long nativePtr, @NonNull Object obj, @NonNull Destroyer destroyer) {
        startIfNecessary();

        NativeRefHolder nativeRefHolder = new NativeRefHolder(nativePtr, destroyer);
        NativePhantomReference nativePhantomReference =
                new NativePhantomReference(obj, mReferenceQueue, nativeRefHolder);
        synchronized (mStrongReferencesLock) {
            mStrongReferences.add(nativePhantomReference);
        }
        return nativeRefHolder;
    }

    private void startIfNecessary() {
        synchronized (mThreadLock) {
            if (mThread != null) return;

            mThread = new Thread(new CleanerRunnable(), "NativeRefAssassin");
            mThread.start();
        }
    }

    void stopForTest() {
        synchronized (mThreadLock) {
            mCleanerRunnable.stop();
            if (mThread != null) mThread.interrupt();
            mThread = null;
        }
    }
}
