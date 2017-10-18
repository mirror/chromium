// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.testing.local.LocalRobolectricTestRunner;

import java.lang.ref.WeakReference;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
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
    private static final long FORCE_GARBAGE_COLLECTION_TIMEOUT_MS = 2000;
    private static final long WAIT_TIME_FOR_DESTROYING_OBJECT_MS = 5000;

    // 60Hz ~= 16ms, which should be fast enough for everyone.
    private static final int SPINNING_LOOPER_EMPTIER_SLEEP_MS = 16;

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
                    Thread.sleep(SPINNING_LOOPER_EMPTIER_SLEEP_MS);
                } catch (InterruptedException e) {
                    // The thread this is running on will be interrupted after stop() has been
                    // called, so the loop will quit on the next iteration.
                }
            }
        }

        public void stop() {
            mRunning.set(false);
        }
    }

    /**
     * An example class to be garbage collected.
     */
    private static class TestNativeDestroyable {
        NativeRefHolder mRefHolder;

        TestNativeDestroyable(long nativePtr, Handler destroyHandler) {
            if (destroyHandler.getLooper() == Looper.getMainLooper()) {
                mRefHolder =
                        NativeRefAssassin.getInstance().registerReferenceMainHandlerDestruction(
                                nativePtr, this, TestNativeDestroyable::nativeDestroy);
            } else {
                mRefHolder = NativeRefAssassin.getInstance().registerReference(
                        nativePtr, this, TestNativeDestroyable::nativeDestroy, destroyHandler);
            }
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
        // Need to use System.currentTimeMillis(), since that is in fact incremented automatically
        // even when running under Robolectric.
        long start = System.currentTimeMillis();
        Object pleaseCollectMe = new Object();
        WeakReference<Object> weakRef = new WeakReference<>(pleaseCollectMe);
        pleaseCollectMe = null; // Setting to null should enable garbage collection.

        // Try to garbage collect pleaseCollectMe.
        boolean hasBeenCollected = false;
        boolean hasTimedOut = false;
        while (!hasBeenCollected && !hasTimedOut) {
            Runtime.getRuntime().gc();
            hasBeenCollected = weakRef.get() == null;
            hasTimedOut = System.currentTimeMillis() - start > FORCE_GARBAGE_COLLECTION_TIMEOUT_MS;
        }

        return hasBeenCollected;
    }

    private static void setInstanceForTest(NativeRefAssassin nativeRefAssassin)
            throws ClassNotFoundException, NoSuchFieldException, IllegalAccessException {
        Class<?> lazyHolderClass = Class.forName("org.chromium.base.NativeRefAssassin$LazyHolder");
        Field instanceField = lazyHolderClass.getDeclaredField("INSTANCE");
        instanceField.setAccessible(true);

        // Remove final modifier.
        Field modifiersField = Field.class.getDeclaredField("modifiers");
        modifiersField.setAccessible(true);
        modifiersField.setInt(instanceField, instanceField.getModifiers() & ~Modifier.FINAL);

        instanceField.set(null, nativeRefAssassin);
    }

    // These field track and provide concurrency for destruction of objects. They are kept static
    // to ensure this test class mimics real prod code as much as possible, since developers might
    // look at a test to see how the NativeRefAssassin and NativeRefHolder should be used.
    private static final Object sDestroyedPointersLock = new Object();
    @GuardedBy("sDestroyedPointersLock")
    private static Set<Long> sDestroyedPointers;
    private static Semaphore sSemaphore;

    /**
     * The thread to run the destruction of objects on when not on the main thread.
     */
    private HandlerThread mDestroyingHandlerThread =
            new HandlerThread("TestDestroyingHandlerThread");

    /**
     * The handler to run destruction of objects on when not using the main thread.
     */
    private Handler mDestroyingHandler;

    /**
     * The handler to run destruction of objects on when using the main thread.
     */
    private Handler mMainHandler;

    // Robolectric is unable to automatically executing scheduled tasks when the scheduler thread
    // does not match, so these threads are just spinning to try to empty the looper for the
    // destroying handler.
    private Thread mDestroyingHandlerLooperEmptierThread;
    private RobolectricLooperEmptier mDestroyingHandlerLooperEmptier;
    private Thread mMainHandlerLooperEmptierThread;
    private RobolectricLooperEmptier mMainHandlerLooperEmptier;

    @Before
    public void setUpTest()
            throws IllegalAccessException, NoSuchFieldException, ClassNotFoundException {
        mMainHandler = new Handler(Looper.getMainLooper());

        mDestroyingHandlerThread.start();
        mDestroyingHandler = new Handler(mDestroyingHandlerThread.getLooper());
        Looper destroyingLooper = mDestroyingHandlerThread.getLooper();

        mDestroyingHandlerLooperEmptier = new RobolectricLooperEmptier(destroyingLooper);
        mDestroyingHandlerLooperEmptierThread = new Thread(
                mDestroyingHandlerLooperEmptier, "Task finisher for " + destroyingLooper);
        mDestroyingHandlerLooperEmptierThread.start();

        mMainHandlerLooperEmptier = new RobolectricLooperEmptier(Looper.getMainLooper());
        mMainHandlerLooperEmptierThread =
                new Thread(mMainHandlerLooperEmptier, "Task finisher for main looper");
        mMainHandlerLooperEmptierThread.start();

        synchronized (sDestroyedPointersLock) {
            sDestroyedPointers = new HashSet<>();
        }
        sSemaphore = new Semaphore(0);

        setInstanceForTest(new NativeRefAssassin());
    }

    @After
    public void tearDownTest() throws IllegalAccessException, NoSuchFieldException,
                                      ClassNotFoundException, InterruptedException {
        NativeRefAssassin.getInstance().stopForTest();
        mDestroyingHandlerLooperEmptier.stop();
        mDestroyingHandlerLooperEmptierThread.interrupt();
        mDestroyingHandlerLooperEmptierThread.join();

        mMainHandlerLooperEmptier.stop();
        mMainHandlerLooperEmptierThread.interrupt();
        mMainHandlerLooperEmptierThread.join();

        Assert.assertTrue(mDestroyingHandlerThread.quitSafely());

        setInstanceForTest(null);
    }

    // Generic test to ensure that at least garbage collection works as expected on the host.
    @Test
    public void ensureGarbageCollectionWorks() {
        Assert.assertTrue(tryToForceGarbageCollection());
    }

    @Test
    public void garbageCollectingObjectShouldInvokeDestroy() throws InterruptedException {
        runGarbageCollectingSingleObjectTest(mDestroyingHandler);
    }

    @Test
    public void garbageCollectingObjectShouldInvokeDestroyOnMainThread()
            throws InterruptedException {
        runGarbageCollectingSingleObjectTest(mMainHandler);
    }

    private void runGarbageCollectingSingleObjectTest(Handler destroyingHandler)
            throws InterruptedException {
        // Create a new object, which should register itself with the NativeRefAssassin.
        TestNativeDestroyable obj = new TestNativeDestroyable(42L, destroyingHandler);
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
        runGarbageCollectingMultipleObjectsTest(mDestroyingHandler);
    }

    @Test
    public void garbageCollectingMultipleObjectsShouldInvokeDestroyOnMainThread()
            throws InterruptedException {
        runGarbageCollectingMultipleObjectsTest(mMainHandler);
    }

    private void runGarbageCollectingMultipleObjectsTest(Handler destroyingHandler)
            throws InterruptedException {
        // Create a new object, which should register itself with the NativeRefAssassin.
        TestNativeDestroyable obj1 = new TestNativeDestroyable(42L, destroyingHandler);
        Assert.assertEquals(42L, obj1.mRefHolder.getNativePtr());
        TestNativeDestroyable obj2 = new TestNativeDestroyable(84L, destroyingHandler);
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
