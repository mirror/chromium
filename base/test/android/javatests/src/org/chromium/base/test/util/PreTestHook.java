// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.util;

import android.content.Context;
import android.support.test.InstrumentationRegistry;

import org.junit.rules.TestWatcher;
import org.junit.runner.Description;

/**
 * Check whether a test case should be skipped.
 */
public abstract class PreTestHook extends TestWatcher {

    public abstract void run(Context targetContext, Description desc);

    @Override
    protected void starting(Description desc) {
        super.starting(desc);
        run(InstrumentationRegistry.getTargetContext(), desc);
    }
}
