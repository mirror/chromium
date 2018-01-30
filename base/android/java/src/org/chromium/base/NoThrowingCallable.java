// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

/**
 * A class similar to {@link java.util.concurrent.Callable} but does not throw {@link
 * Exception}.
 * @param V The type of object to return after running the callable.
 */
public interface NoThrowingCallable<V> {
    /**
     * Computes a result from executing a task.
     * @return computed result
     */
    V call();
}
