// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.os.Handler;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.testing.local.LocalRobolectricTestRunner;

import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Test for {@link NativeRefHolder}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class NativeRefHolderTest {
    @Test
    public void nativePtrIsSetCorrectly() {
        NativeRefHolder<Object> refHolder =
                new NativeRefHolder<>(42, nativePtr -> {}, new Handler());
        Assert.assertEquals(42, refHolder.getNativePtr());
    }

    @Test
    public void invokingDestroyInvokesDestroyerAndClearsNativePtr() {
        final AtomicLong destroyedPtr = new AtomicLong();
        final AtomicBoolean destroyed = new AtomicBoolean(false);

        // Ensure that no posted tasks are automatically executed.
        Handler destroyHandler = new Handler();
        ShadowLooper destroyLooper = Shadows.shadowOf(destroyHandler.getLooper());
        destroyLooper.pause();

        NativeRefHolder<Object> refHolder = new NativeRefHolder<>(42, nativePtr -> {
            destroyedPtr.set(nativePtr);
            destroyed.set(true);
        }, destroyHandler);

        // Destroying should clear the native pointer and post a task to invoke the Destroyer.
        refHolder.destroy();
        Assert.assertEquals(0L, refHolder.getNativePtr());
        Assert.assertFalse(destroyed.get());

        // Destroyer should have been invoked after the looper has run to the end of its tasks.
        destroyLooper.runToEndOfTasks();
        Assert.assertEquals(0L, refHolder.getNativePtr());
        Assert.assertTrue(destroyed.get());
    }

    @Test
    public void invokingDestroyTwiceInvokesDestroyerOnlyOnce() {
        final AtomicLong destroyedPtr = new AtomicLong();
        final AtomicLong destroyCounter = new AtomicLong(0L);

        // Ensure that no posted tasks are automatically executed.
        Handler destroyHandler = new Handler();
        ShadowLooper destroyLooper = Shadows.shadowOf(destroyHandler.getLooper());
        destroyLooper.pause();

        NativeRefHolder<Object> refHolder = new NativeRefHolder<>(42, nativePtr -> {
            destroyedPtr.set(nativePtr);
            destroyCounter.incrementAndGet();
        }, destroyHandler);

        // Destroying should clear the native pointer and post a task to invoke the Destroyer.
        refHolder.destroy();
        Assert.assertEquals(0L, refHolder.getNativePtr());
        Assert.assertEquals(0L, destroyCounter.get());

        // Destroyer should have been invoked after the looper has run to the end of its tasks.
        destroyLooper.runToEndOfTasks();
        Assert.assertEquals(0L, refHolder.getNativePtr());
        Assert.assertEquals(42L, destroyedPtr.get());
        Assert.assertEquals(1L, destroyCounter.get());

        // Destroying again should not lead to any changes.
        refHolder.destroy();
        destroyLooper.runToEndOfTasks();
        Assert.assertEquals(0L, refHolder.getNativePtr());
        Assert.assertEquals(42L, destroyedPtr.get());
        Assert.assertEquals(1L, destroyCounter.get());
    }
}
