// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.android_webview_glue;

import com.android.webviewsupportlib.WebViewProviderFactory;

import org.chromium.android_webview.SharedWebViewProviderState;
import org.chromium.base.Log;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;

class GlueWebViewChromiumProviderFactory implements WebViewProviderFactory {
    private final static String TAG = GlueWebViewChromiumProviderFactory.class.getSimpleName();

    private GlueWebViewChromiumProviderFactory() {}

    public static GlueWebViewChromiumProviderFactory create() {
        return new GlueWebViewChromiumProviderFactory();
    }

    //public Statics getStatics() {
    //    return null;
    //}

    public InvocationHandler createWebView(android.webkit.WebViewProvider webviewProvider) {
        SharedWebViewProviderState webviewChromium = (SharedWebViewProviderState) webviewProvider;

        final GlueWebViewChromiumProvider provider =
                new GlueWebViewChromiumProvider(webviewChromium);
        return new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
                return GlueFactoryProviderFetcher.dupeMethod(method).invoke(provider, objects);
            }
        };
    }

    //public GeolocationPermissions getGeolocationPermissions() {
    //    return null;
    //}

    //public CookieManager getCookieManager() {
    //    return null;
    //}

    //public TokenBindingService getTokenBindingService() {
    //    return null;
    //}

    //public ServiceWorkerController getServiceWorkerController() {
    //    return null;
    //}

    //public WebIconDatabase getWebIconDatabase() {
    //    return null;
    //}

    //public WebStorage getWebStorage() {
    //    return null;
    //}

    //public WebViewDatabase getWebViewDatabase(Context context) {
    //    return null;
    //}
}
