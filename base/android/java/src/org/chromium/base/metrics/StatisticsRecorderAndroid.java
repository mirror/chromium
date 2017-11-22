// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.metrics;

import org.chromium.base.annotations.JNINamespace;

/**
 * Java API which exposes the registered histograms on the native side as
 * JSON test.
 */
@JNINamespace("base::android")
public final class StatisticsRecorderAndroid {
    private StatisticsRecorderAndroid() {}

    /**
     * @param omitBuckets Set to true if the serialization of buckets should be omitted during the
     * serialization of the histograms. Should be set to true when it is critical to reduce the
     * memory pressure during the serialization.
     * @return All the registered histograms as JSON text.
     */
    public static String toJson(boolean omitBuckets) {
        return nativeToJson(omitBuckets);
    }

    private static native String nativeToJson(boolean omitBuckets);
}