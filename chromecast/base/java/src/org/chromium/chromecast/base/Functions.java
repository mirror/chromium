// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

/**
 * Generic signatures for functional interfaces.
 *
 * TODO(sanfin): replace with Java 8 library functions if we're ever able to use the Java 8 library.
 */
public class Functions {
    /**
     * A function that takes a single argument and returns void.
     *
     * @param <T> The argument type.
     */
    public interface Consumer<T> { public void accept(T value); }

    /**
     * A function that takes a single argument and returns a value.
     *
     * @param <T> The argument type.
     * @param <R> The return type.
     */
    public interface Function<T, R> { public R apply(T input); }

    /**
     * A function that takes a single argument and returns a boolean.
     *
     * @param <T> The argument type.
     */
    public interface Predicate<T> { public boolean test(T value); }
}
