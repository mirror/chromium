// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.webkit.SafeBrowsingResponse;
import android.webkit.WebView;

import com.android.webview.chromium.WebViewDelegateFactory.WebViewDelegate;

import org.chromium.android_webview.AwSafeBrowsingResponse;
import org.chromium.android_webview.SafeBrowsingAction;
import org.chromium.base.Callback;

@TargetApi(Build.VERSION_CODES.O_MR1)
class WebViewContentsClientAdapterForOMR1 extends WebViewContentsClientAdapterForO {
    WebViewContentsClientAdapterForOMR1(
            WebView webView, Context context, WebViewDelegate webViewDelegate) {
        super(webView, context, webViewDelegate);
    }

    @Override
    public void onSafeBrowsingHit(AwWebResourceRequest request, int threatType,
            final Callback<AwSafeBrowsingResponse> callback) {
        mWebViewClient.onSafeBrowsingHit(mWebView, new WebResourceRequestImpl(request), threatType,
                new SafeBrowsingResponse() {
                    @Override
                    public void showInterstitial(boolean allowReporting) {
                        callback.onResult(new AwSafeBrowsingResponse(
                                SafeBrowsingAction.SHOW_INTERSTITIAL, allowReporting));
                    }

                    @Override
                    public void proceed(boolean report) {
                        callback.onResult(
                                new AwSafeBrowsingResponse(SafeBrowsingAction.PROCEED, report));
                    }

                    @Override
                    public void backToSafety(boolean report) {
                        callback.onResult(new AwSafeBrowsingResponse(
                                SafeBrowsingAction.BACK_TO_SAFETY, report));
                    }
                });
    }
}
