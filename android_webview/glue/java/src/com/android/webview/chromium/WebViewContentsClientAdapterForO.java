// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.webkit.RenderProcessGoneDetail;
import android.webkit.WebView;

import com.android.webview.chromium.WebViewDelegateFactory.WebViewDelegate;

import org.chromium.android_webview.AwRenderProcessGoneDetail;
import org.chromium.base.TraceEvent;

@TargetApi(Build.VERSION_CODES.O)
class WebViewContentsClientAdapterForO extends WebViewContentsClientAdapter {
    WebViewContentsClientAdapterForO(
            WebView webView, Context context, WebViewDelegate webViewDelegate) {
        super(webView, context, webViewDelegate);
    }

    @Override
    public boolean onRenderProcessGone(final AwRenderProcessGoneDetail detail) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onRenderProcessGone");
            return mWebViewClient.onRenderProcessGone(mWebView, new RenderProcessGoneDetail() {
                @Override
                public boolean didCrash() {
                    return detail.didCrash();
                }

                @Override
                public int rendererPriorityAtExit() {
                    return detail.rendererPriority();
                }
            });
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onRenderProcessGone");
        }
    }
}
