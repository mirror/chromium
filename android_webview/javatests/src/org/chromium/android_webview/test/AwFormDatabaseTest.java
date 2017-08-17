// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.support.test.filters.SmallTest;

import org.chromium.android_webview.AwFormDatabase;

/** AwFormDatabaseTest. */
public class AwFormDatabaseTest extends AwTestBase {
    @SmallTest
    public void testSmoke() throws Throwable {
        runTestOnUiThread(() -> {
            AwFormDatabase.clearFormData();
            assertFalse(AwFormDatabase.hasFormData());
        });
    }
}
