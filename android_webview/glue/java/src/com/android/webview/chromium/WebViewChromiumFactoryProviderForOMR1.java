// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.content.Context;
import android.webkit.WebView;

class WebViewChromiumFactoryProviderForOMR1 extends WebViewChromiumFactoryProviderForO {
    public static WebViewChromiumFactoryProvider create(android.webkit.WebViewDelegate delegate) {
        return new WebViewChromiumFactoryProviderForOMR1(delegate);
    }

    @Override
    WebViewContentsClientAdapter createWebViewContentsClientAdapter(
            WebView webView, Context context) {
        return new WebViewContentsClientAdapterForOMR1(webView, context, getWebViewDelegate());
    }

    protected WebViewChromiumFactoryProviderForOMR1(android.webkit.WebViewDelegate delegate) {
        super(delegate);
    }
}
