// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.content.Context;
import android.webkit.WebView;

class WebViewChromiumFactoryProviderForO extends WebViewChromiumFactoryProvider {
    public static WebViewChromiumFactoryProvider create(android.webkit.WebViewDelegate delegate) {
        return new WebViewChromiumFactoryProviderForO(delegate);
    }

    @Override
    WebViewContentsClientAdapter createWebViewContentsClientAdapter(
            WebView webView, Context context) {
        return new WebViewContentsClientAdapterForO(webView, context, getWebViewDelegate());
    }

    protected WebViewChromiumFactoryProviderForO(android.webkit.WebViewDelegate delegate) {
        super(delegate);
    }
}
