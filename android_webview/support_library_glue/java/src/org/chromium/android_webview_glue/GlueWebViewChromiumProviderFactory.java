// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.android_webview_glue;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.view.ViewGroup;
// TODO all of these com.android.webviewsupportlib imports should have been duped into the support
// library so we use classes from there instead.
// TODO find a good way of duping such classes into the support library.
// Again: problem with these classes being different in Support Library, and in WebView APK
// (support-lib glue).
import com.android.webviewsupportlib.CookieManager;
import com.android.webviewsupportlib.GeolocationPermissions;
import com.android.webviewsupportlib.ServiceWorkerController;
import com.android.webviewsupportlib.TokenBindingService;
import com.android.webviewsupportlib.ValueCallback;
import com.android.webviewsupportlib.WebIconDatabase;
import com.android.webviewsupportlib.WebStorage;
import com.android.webviewsupportlib.WebViewDatabase;

import org.chromium.base.Log;

import com.android.webviewsupportlib.WebView;
import com.android.webviewsupportlib.WebViewProviderFactory;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

// Implementation of GlueWebViewProviderFactory - this is used by an InvocationHandler in
// GlueFactoryProviderFetcher to call provide sort of WebViewFactoryProvider implementation to the
// support library.
// TODO the naming ffs..
class GlueWebViewChromiumProviderFactory implements WebViewProviderFactory {
    private final static String TAG = GlueWebViewChromiumProviderFactory.class.getSimpleName();

    private GlueWebViewChromiumProviderFactory() {}

    public static GlueWebViewChromiumProviderFactory create() {
        return new GlueWebViewChromiumProviderFactory();
    }

    // TODO all of the below

    public Statics getStatics() {
        return null;
    }

    public static <T> T castToSuppLibClass(Class<T> clazz, InvocationHandler invocationHandler) {
        return clazz.cast(
                Proxy.newProxyInstance(GlueWebViewChromiumProviderFactory.class.getClassLoader(),
                        new Class[] {clazz}, invocationHandler));
    }

    public InvocationHandler createWebView(InvocationHandler webviewFake,
            InvocationHandler webviewPrivateFake, ViewGroup containerView, Context context) {
        // WebView webview = castToSuppLibClass(WebView.class, webviewFake);
        // WebView.PrivateAccess webviewPrivate =
        //        castToSuppLibClass(WebView.PrivateAccess.class, webviewPrivateFake);

        // TODO shouldDisableThreadChecking
        // final GlueWebViewProvider provider = new GlueWebViewProvider(this, webview,
        // webviewPrivate);
        final GlueWebViewProvider provider = new GlueWebViewProvider(this, containerView, context);
        return new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
                // Log.e(TAG, "in InvocationHandler.invoke for " + o + " method: " + method);
                // TODO don't forget to hook this up with a Proxy on the support library
                // side - having this implement the supp-lib version of the WebViewFactory
                // interface (i.e. the one in that ClassLoader).
                return GlueFactoryProviderFetcher.dupeMethod(method).invoke(provider, objects);
            }
        };
    }

    // TODO the entire ensureChromiumStartedLocked logic should probably be moved out into its own
    // component - so that this class simply contains ctors, and getters for static objects.

    public GeolocationPermissions getGeolocationPermissions() {
        return null;
    }

    public CookieManager getCookieManager() {
        return null;
    }

    public TokenBindingService getTokenBindingService() {
        return null;
    }

    public ServiceWorkerController getServiceWorkerController() {
        return null;
    }

    public WebIconDatabase getWebIconDatabase() {
        return null;
    }

    public WebStorage getWebStorage() {
        return null;
    }

    public WebViewDatabase getWebViewDatabase(Context context) {
        return null;
    }
}
