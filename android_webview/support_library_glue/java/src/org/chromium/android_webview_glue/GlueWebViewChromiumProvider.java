// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.android_webview_glue;

import android.util.Log;

import com.android.webviewsupportlib.AugmentedWebView;
import com.android.webviewsupportlib.WebViewProvider;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.SharedWebViewProviderState;

public class GlueWebViewChromiumProvider implements WebViewProvider {
    private static final String TAG = GlueWebViewChromiumProvider.class.getSimpleName();

    //private AwSettings mAwSettings;
    // The WebView wrapper for ContentViewCore and required browser compontents.
    SharedWebViewProviderState mSharedState;

    public GlueWebViewChromiumProvider(SharedWebViewProviderState sharedState) {
        mSharedState = sharedState;
        Log.e(TAG, "in GlueWebViewChromiumProvider ctor");
    }

    // TODO is there anything in the init-methods that should differ between WebViewChromium and
    // this class?
    //          Should simply creating an AugmentedWebView affect the state of the initial WebView?

    public void insertVisualStateCallback(long requestId, InvocationHandler callbackInvoHandler) {
        final AugmentedWebView.VisualStateCallback visualStateCallback =
                GlueFactoryProviderFetcher.castToSuppLibClass(
                        AugmentedWebView.VisualStateCallback.class, callbackInvoHandler);

        mSharedState.getAwContents().insertVisualStateCallback(requestId,
                new AwContents.VisualStateCallback() {
                        @Override
                        public void onComplete(long requestId) {
                            visualStateCallback.onComplete(requestId);
                        }
                });
    }


    //@Override
    //public void setWebViewClient(InvocationHandler invocationHandler) {
    //    Log.e(TAG, "in setWebViewClient");
    //    WebViewClientInterface webviewClient = (WebViewClientInterface) Proxy.newProxyInstance(
    //            GlueWebViewProvider.class.getClassLoader(),
    //            new Class[] {WebViewClientInterface.class}, invocationHandler);
    //    mContentsClientAdapter.setWebViewClient(webviewClient);
    //}
}
