// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feedback;

import android.graphics.Bitmap;

import org.chromium.base.ThreadUtils;

class StaticScreenshotSource implements ScreenshotSource {
    private final Bitmap mBitmap;

    public StaticScreenshotSource(Bitmap screenshot) {
        mBitmap = screenshot;
    }

    // ScreenshotSource implementation.
    @Override
    public void capture(Runnable callback) {
        ThreadUtils.postOnUiThread(callback);
    }

    @Override
    public boolean isReady() {
        return true;
    }

    @Override
    public Bitmap getScreenshot() {
        return mBitmap;
    }
}