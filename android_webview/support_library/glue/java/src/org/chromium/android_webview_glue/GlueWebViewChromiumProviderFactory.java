// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.android_webview_glue;

import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewProvider;

import com.android.webview.chromium.ContentSettingsAdapter;
import com.android.webview.chromium.WebViewChromiumFactoryProviderSingleton;

import org.chromium.android_webview.SharedWebViewProviderState;
import org.chromium.base.Log;
import org.chromium.support_lib_boundary.WebViewProviderFactoryBoundaryInterface;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;

class GlueWebViewChromiumProviderFactory implements WebViewProviderFactoryBoundaryInterface {
    private final static String TAG = GlueWebViewChromiumProviderFactory.class.getSimpleName();

    private GlueWCFPStaticsAdapter mStatics;

    private GlueWebViewChromiumProviderFactory() {}

    public static GlueWebViewChromiumProviderFactory create() {
        return new GlueWebViewChromiumProviderFactory();
    }

    @Override
    public InvocationHandler createWebView(WebView webview) {
        final GlueWebViewChromiumProvider provider =
                WebkitObjectConverter.convertWebViewProvider(webview.getWebViewProvider());
        return new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
                return GlueFactoryProviderFetcher.dupeMethod(method).invoke(provider, objects);
            }
        };
    }

    @Override
    public InvocationHandler getStatics() {
        // TODO get WVCFP from the singleton
        // TODO then fetch Statics from that (this isn't necessary in the prototype).
        // TODO then create StaticsAdapter from that fetched Statics

        // TODO trigger WebViewChromiumFactoryProvider lazy initialization.
        return GlueFactoryProviderFetcher.createInvocationHandlerFor(
                new GlueWCFPStaticsAdapter(WebViewChromiumFactoryProviderSingleton.getProvider()));
    }

    @Override
    public InvocationHandler getCompatFetcher() {
        // TODO set the private compat fetcher
        // TODO trigger lazy init?
        return GlueFactoryProviderFetcher.createInvocationHandlerFor(new CompatFetcher());
    }

    //@Override
    // public InvocationHandler getSupLibWebSettings(WebSettings webSettings) {
    //    // TODO
    //    if (webSettings instanceof ContentSettingsAdapter) {
    //        Log.e(TAG,
    //            "in getSupLibWebSettings, webSettings is an (old-glue) ContentSettingsAdapter");
    //        ContentSettingsAdapter settingsAdapter = (ContentSettingsAdapter) webSettings;
    //        return GlueFactoryProviderFetcher.createInvocationHandlerFor(
    //                new GlueWebSettingsAdapter(settingsAdapter.getAwSettings()));
    //        // TODO fetch AwSettings from ContentSettingsAdapter
    //    //} else if (webSettings instanceof GlueWebSettingsAdapter) { // TODO these classes are
    //    not
    //    //even compatible.. do we have a case like this...?
    //    //    Log.e(TAG,
    //    //        "in getSupLibWebSettings, webSettings is a (sup-lib-glue)
    //    GlueWebSettingsAdapter");
    //    //    return GlueFactoryProviderFetcher.createInvocationHandlerFor(webSettings);
    //    } else {
    //        throw new RuntimeException("Unrecognized WebSettings object.");
    //    }
    //}
}
