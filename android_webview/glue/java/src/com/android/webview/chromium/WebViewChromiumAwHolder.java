// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import org.chromium.android_webview.AwTracingController;

class WebViewChromiumAwHolder {
    private final WebViewChromiumFactoryProvider mFactory;

    WebViewChromiumAwHolder(final WebViewChromiumFactoryProvider factory) {
        mFactory = factory;
    }

    // Allows down-stream to override this.
    protected void startChromiumLocked() {}

    AwTracingController getTracingControllerOnUiThread() {
        synchronized (mFactory.mLock) {
            mFactory.ensureChromiumStartedLocked(true);
            return mFactory.getBrowserContextOnUiThread().getTracingController();
        }
    }
}
