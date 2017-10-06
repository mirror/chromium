// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

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
        NativeRefHolder refHolder = new NativeRefHolder(42, nativePtr -> {});
        Assert.assertEquals(42, refHolder.getNativePtr());
    }

    @Test
    public void invokingDestroyInvokesDestroyerAndClearsNativePtr() {
        final AtomicLong destroyedPtr = new AtomicLong();
        final AtomicBoolean destroyed = new AtomicBoolean(false);
        NativeRefHolder refHolder = new NativeRefHolder(42, nativePtr -> {
            destroyedPtr.set(nativePtr);
            destroyed.set(true);
        });

        // Destroying should clear native ptr and invoke destroyer.
        refHolder.destroy();
        Assert.assertEquals(0L, refHolder.getNativePtr());
        Assert.assertTrue(destroyed.get());
    }

    @Test
    public void invokingDestroyTwiceInvokesDestroyerOnlyOnce() {
        final AtomicLong destroyedPtr = new AtomicLong();
        final AtomicLong destroyCounter = new AtomicLong(0L);
        NativeRefHolder refHolder = new NativeRefHolder(42, nativePtr -> {
            destroyedPtr.set(nativePtr);
            destroyCounter.incrementAndGet();
        });

        // Destroying once should clear native ptr and invoke destroyer.
        refHolder.destroy();
        Assert.assertEquals(0L, refHolder.getNativePtr());
        Assert.assertEquals(42L, destroyedPtr.get());
        Assert.assertEquals(1L, destroyCounter.get());

        // Destroying again should not lead to any changes.
        refHolder.destroy();
        Assert.assertEquals(0L, refHolder.getNativePtr());
        Assert.assertEquals(42L, destroyedPtr.get());
        Assert.assertEquals(1L, destroyCounter.get());
    }
}
