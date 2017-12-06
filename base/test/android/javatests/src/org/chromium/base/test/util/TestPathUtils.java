// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.util;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.MainDex;

@JNINamespace("base::android")
/**
 * Utility class to provide path for any process.
 */
@MainDex
class TestPathUtils {
    /**
     * Get the isolated test root directory.
     */
    @CalledByNative
    public static String getIsolatedTestRoot() {
        return UrlUtils.getIsolatedTestRoot();
    }
}
