// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.android_webview_glue;

import android.webkit.WebSettings;
import android.webkit.WebView;

import com.android.webview.chromium.ContentSettingsAdapter;
import com.android.webview.chromium.WebViewChromium;

import org.chromium.support_lib_boundary.CompatFetcherBoundaryInterface;

import java.lang.reflect.InvocationHandler;

// TODO comments
class CompatFetcher implements CompatFetcherBoundaryInterface {
    CompatFetcher() {}

    public Object getAttachedCompat(WebView webview) {
        WebViewChromium provider = (WebViewChromium) webview.getWebViewProvider();
        return provider.getAttachedCompat();
    }

    public void attachCompat(WebView webview, Object webviewProviderAdapter) {
        WebViewChromium provider = (WebViewChromium) webview.getWebViewProvider();
        provider.attachCompat(webviewProviderAdapter);
    }

    public Object getAttachedCompat(WebSettings webSettings) {
        ContentSettingsAdapter adapter = (ContentSettingsAdapter) webSettings;
        return adapter.getAttachedCompat();
    }

    public void attachCompat(WebSettings webSettings, Object webSettingsAdapter) {
        ContentSettingsAdapter adapter = (ContentSettingsAdapter) webSettings;
        adapter.attachCompat(webSettingsAdapter);
    }

    public InvocationHandler createCompat(WebSettings webSettings) {
        ContentSettingsAdapter adapter = (ContentSettingsAdapter) webSettings;
        return GlueFactoryProviderFetcher.createInvocationHandlerFor(
                new GlueWebSettingsAdapter(adapter.getAwSettings()));
    }
}
