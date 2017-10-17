// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.util;

import org.junit.Assume;
import org.junit.rules.TestWatcher;
import org.junit.runner.Description;

/**
 * Check whether a test case should be skipped.
 */
public abstract class SkipRule extends TestWatcher {
    /**
     *
     * Checks whether the given test method should be skipped.
     *
     * @return Whether the test case should be skipped.
     */
    public abstract boolean shouldSkip(Description desc);

    @Override
    protected void starting(Description desc) {
        super.starting(desc);
        Assume.assumeFalse("Test should be skipped", shouldSkip(desc));
    }
}
