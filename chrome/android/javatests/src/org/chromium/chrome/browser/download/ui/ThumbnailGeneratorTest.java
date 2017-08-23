// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.support.test.filters.SmallTest;

import org.junit.Test;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.test.ChromeActivityTestCaseBase;

/**
 * Tests {@link ThumbnailGenerator}.
 */
public class ThumbnailGeneratorTest extends ChromeActivityTestCaseBase<ChromeActivity> {
    public ThumbnailGeneratorTest() {
        super(ChromeActivity.class);
    }

    @Override
    public void startMainActivity() throws InterruptedException {
        startMainActivityOnBlankPage();
    }

    /**
     * Verifies that destroying without first calling {@link ThumbnailGenerator#retrieveThumbnail
     * (ThumbnailProvider.ThumbnailRequest,
     * ThumbnailGeneratorCallback)} works, and that any number of calls to {@link
     * ThumbnailGenerator#destroy()} can be happen without a crash.
     */
    @Test
    @SmallTest
    public void testConstructAndDestroy() {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                ThumbnailGenerator generator = new ThumbnailGenerator();
                generator.destroy();
                generator.destroy();
            }
        });
    }
}
