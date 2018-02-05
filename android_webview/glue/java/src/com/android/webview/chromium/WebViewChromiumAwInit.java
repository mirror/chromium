// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

class WebViewChromiumAwInit {
    protected final WebViewChromiumFactoryProvider mFactory;

    WebViewChromiumAwInit(WebViewChromiumFactoryProvider factory) {
        mFactory = factory;
        // Do not make calls into 'factory' in this ctor - this ctor is called from the
        // WebViewChromiumFactoryProvider ctor, so 'factory' is not properly initialized yet.
    }

    // Allows down-stream to override this.
    protected void startChromiumLocked() {}
}
