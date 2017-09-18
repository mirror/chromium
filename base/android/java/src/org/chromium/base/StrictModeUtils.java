// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.os.StrictMode;

import java.io.Closeable;

/**
 * This class provides convenience methods for permitting StrictMode violations.
 */
public final class StrictModeUtils {
    private StrictModeUtils() {}

    /**
     * Convenience method for disabling StrictMode for disk-writes with try-with-resources.
     */
    public static PolicyHelper allowDiskWrites() {
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskWrites();
        return new PolicyHelper(oldPolicy);
    }

    /**
     * Convenience method for disabling StrictMode for disk-reads with try-with-resources.
     */
    public static PolicyHelper allowDiskReads() {
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        return new PolicyHelper(oldPolicy);
    }

    /**
     * Enables try-with-resources compatible StrictMode violation whitelisting.
     *
     * Example:
     * <pre>
     *     try (StrictModeUtils.PolicyHelper unused = StrictModeUtils.allowDiskWrites()) {
     *         return Example.doThingThatRequiresDiskWrites();
     *     }
     * </pre>
     *
     */
    public static final class PolicyHelper implements Closeable {
        private final StrictMode.ThreadPolicy mThreadPolicy;

        private PolicyHelper(StrictMode.ThreadPolicy threadPolicy) {
            mThreadPolicy = threadPolicy;
        }

        @Override
        public void close() {
            StrictMode.setThreadPolicy(mThreadPolicy);
        }
    }
}