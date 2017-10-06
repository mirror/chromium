// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.SystemClock;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.base.test.util.ScalableTimeout;
import org.chromium.testing.local.LocalRobolectricTestRunner;

import java.lang.ref.WeakReference;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

import javax.annotation.concurrent.GuardedBy;

/**
 * Test for {@link NativeRefAssassin}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class NativeRefAssassinTest {
    private static final long FORCE_GARBAGE_COLLECTION_TIMEOUT_MS =
            ScalableTimeout.scaleTimeout(2000);
    private static final long WAIT_TIME_FOR_DESTROYING_OBJECT_MS =
            ScalableTimeout.scaleTimeout(5000);

    /**
     * A stoppable, spinning {@link Runnable} which constantly empties the task queue of the
     * given {@link Looper}.
     */
    private static class RobolectricLooperEmptier implements Runnable {
        private final AtomicBoolean mRunning = new AtomicBoolean(true);
        private final ShadowLooper mShadowLooper;

        RobolectricLooperEmptier(Looper looper) {
            mShadowLooper = Shadows.shadowOf(looper);
        }

        @Override
        public void run() {
            while (mRunning.get()) {
                mShadowLooper.runToEndOfTasks();
                try {
                    Thread.sleep(16);
                } catch (InterruptedException e) {
                    // Intentionally ignore.
                }
            }
        }

        public void stop() {
            mRunning.set(true);
        }
    }

    /**
     * An example class to be garbage collected.
     */
    private static class TestNativeDestroyable {
        NativeRefHolder mRefHolder;

        TestNativeDestroyable(long nativePtr) {
            mRefHolder = NativeRefAssassin.getInstance().registerReference(
                    nativePtr, this, TestNativeDestroyable::nativeDestroy);
        }

        /**
         * Counts destruction of native pointers. In prod code, this would typically be a native
         * (JNI) method.
         *
         * IMPORTANT: This must not have a reference back to the TestNativeDestroyable object that
         * should be garbage collected. In practice this means that this method should be static.
         *
         * @param nativePtr the native pointer to destroy.
         */
        private static void nativeDestroy(long nativePtr) {
            synchronized (sDestroyedPointersLock) {
                Assert.assertFalse("nativePtr " + nativePtr + " must only be destroyed once.",
                        sDestroyedPointers.contains(nativePtr));

                sDestroyedPointers.add(nativePtr);
            }

            sSemaphore.release();
        }
    }

    /**
     * @param nativePtr the native pointer to look for.
     * @return whether a native pointer has been destructed.
     */
    private static boolean isDestroyed(long nativePtr) {
        synchronized (sDestroyedPointersLock) {
            return sDestroyedPointers.contains(nativePtr);
        }
    }

    /**
     * Helper method for running garbage collection at least once.
     *
     * @return whether the GC finished successfully within
     * {@link #FORCE_GARBAGE_COLLECTION_TIMEOUT_MS}.
     */
    private static boolean tryToForceGarbageCollection() {
        long start = SystemClock.elapsedRealtime();
        Object pleaseCollectMe = new Object();
        WeakReference<Object> weakRef = new WeakReference<>(pleaseCollectMe);
        pleaseCollectMe = null; // Setting to null should enable garbage collection.
        while (weakRef.get() != null
                || SystemClock.elapsedRealtime() - start > FORCE_GARBAGE_COLLECTION_TIMEOUT_MS) {
            Runtime.getRuntime().gc();
        }
        return weakRef.get() == null;
    }

    // These field track and provide concurrency for destruction of objects. They are kept static
    // to ensure this test class mimics real prod code as much as possible, since developers might
    // look at a test to see how the NativeRefAssassin and NativeRefHolder should be used.
    private static final Object sDestroyedPointersLock = new Object();
    @GuardedBy("sDestroyedPointersLock")
    private static Set<Long> sDestroyedPointers;
    private static Semaphore sSemaphore;

    // The thread to run the destruction of objects on.
    private HandlerThread mDestroyingHandlerThread =
            new HandlerThread("TestDestroyingHandlerThread");

    // Robolectric is unable to automatically executing scheduled tasks when the scheduler thread
    // does not match, so this thread is just spinning to try to empty the looper for the destroying
    // handler.
    private Thread mDestroyingHandlerLooperEmptierThread;
    private RobolectricLooperEmptier mDestroyingHandlerLooperEmptier;

    @Before
    public void setUpTest() {
        mDestroyingHandlerThread.start();
        Handler destroyingHandler = new Handler(mDestroyingHandlerThread.getLooper());
        Looper mDestroyingLooper = mDestroyingHandlerThread.getLooper();

        mDestroyingHandlerLooperEmptier = new RobolectricLooperEmptier(mDestroyingLooper);
        mDestroyingHandlerLooperEmptierThread = new Thread(
                mDestroyingHandlerLooperEmptier, "Task finisher for " + mDestroyingLooper);
        mDestroyingHandlerLooperEmptierThread.start();

        synchronized (sDestroyedPointersLock) {
            sDestroyedPointers = new HashSet<>();
        }
        sSemaphore = new Semaphore(0);

        NativeRefAssassin.setInstanceForTest(new NativeRefAssassin(destroyingHandler));
    }

    @After
    public void tearDownTest() {
        NativeRefAssassin.getInstance().stopForTest();
        mDestroyingHandlerLooperEmptier.stop();
        mDestroyingHandlerLooperEmptierThread.interrupt();

        mDestroyingHandlerThread.quit();
    }

    // Generic test to ensure that at least garbage collection works as expected on the host.
    @Test
    public void ensureGarbageCollectionWorks() {
        Assert.assertTrue(tryToForceGarbageCollection());
    }

    @Test
    public void garbageCollectingObjectShouldInvokeDestroy() throws InterruptedException {
        // Create a new object, which should register itself with the NativeRefAssassin.
        TestNativeDestroyable obj = new TestNativeDestroyable(42L);
        Assert.assertEquals(42L, obj.mRefHolder.getNativePtr());

        // Clear the reference and strongly suggest that a GC should run at least once.
        // This should end up invoking the onDestroy method.
        obj = null;
        Assert.assertTrue(tryToForceGarbageCollection());

        // After a GC has been executed, the RefHolder should have been destroyed.
        Assert.assertTrue(
                sSemaphore.tryAcquire(WAIT_TIME_FOR_DESTROYING_OBJECT_MS, TimeUnit.MILLISECONDS));
        Assert.assertTrue(isDestroyed(42));
    }

    @Test
    public void garbageCollectingMultipleObjectsShouldInvokeDestroy() throws InterruptedException {
        // Create a new object, which should register itself with the NativeRefAssassin.
        TestNativeDestroyable obj1 = new TestNativeDestroyable(42L);
        Assert.assertEquals(42L, obj1.mRefHolder.getNativePtr());
        TestNativeDestroyable obj2 = new TestNativeDestroyable(84L);
        Assert.assertEquals(84L, obj2.mRefHolder.getNativePtr());

        // Clear the references and strongly suggest that a GC should run at least once.
        // This should end up invoking the onDestroy method.
        obj1 = null;
        obj2 = null;
        Assert.assertTrue(tryToForceGarbageCollection());

        // After a GC has been executed, the RefHolder should have been destroyed.
        // Need to wait for 2 semaphore permits.
        Assert.assertTrue(sSemaphore.tryAcquire(
                2, WAIT_TIME_FOR_DESTROYING_OBJECT_MS, TimeUnit.MILLISECONDS));
        Assert.assertTrue(isDestroyed(42));
        Assert.assertTrue(isDestroyed(84));
    }
}
