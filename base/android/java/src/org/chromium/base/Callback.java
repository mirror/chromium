// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import org.chromium.base.annotations.CalledByNative;

/**
 * A simple single-argument callback to handle the result of a computation.
 *
 * @param <T> The type of the computation's result.
 */
public interface Callback<T> {
    /**
     * Invoked with the result of a computation.
     */
    void onResult(T result);

    @SuppressWarnings("unchecked")
    @CalledByNative
    static void onResultFromNative(Callback callback, Object result) {
        callback.onResult(result);
    }

    @SuppressWarnings("unchecked")
    @CalledByNative
    static void onResultFromNative(Callback callback, boolean result) {
        callback.onResult(Boolean.valueOf(result));
    }

    @SuppressWarnings("unchecked")
    @CalledByNative
    static void onResultFromNative(Callback callback, int result) {
        callback.onResult(Integer.valueOf(result));
    }

    @SuppressWarnings("unchecked")
    @CalledByNative
    static void onResultFromNative(Callback callback, byte[] result) {
        callback.onResult(result);
    }
}
