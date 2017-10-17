// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.util;

import org.junit.rules.TestWatcher;
import org.junit.runner.Description;

import org.chromium.base.Log;

public class CommandLineTestRule extends TestWatcher {
    public CommandLineTestRule(String flag, String flags) {
        super(); // STUB
    }

    @Override
    public void starting(Description description) {
        Log.d("#YOLAND", "CommandLineTestRule");
    }
}
