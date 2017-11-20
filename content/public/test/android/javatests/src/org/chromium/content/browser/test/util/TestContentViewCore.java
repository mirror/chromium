// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.test.util;

import android.content.Context;

import org.chromium.content.browser.ContentViewCoreImpl;

/**
 * {@link ContentViewCore} implementation that can be overriden by tests
 * to customize behavior.
 */
public class TestContentViewCore extends ContentViewCoreImpl {
    public TestContentViewCore(Context context, String productVersion) {
        super(context, productVersion);
    }
}
