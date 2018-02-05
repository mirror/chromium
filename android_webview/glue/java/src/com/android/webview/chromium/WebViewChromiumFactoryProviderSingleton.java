// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

/**
 * Global thread-safe storage class for the one and only WebViewChromiumFactoryProvider.
 */
public class WebViewChromiumFactoryProviderSingleton {
    final static Object sLock = new Object();
    static WebViewChromiumFactoryProvider sProvider;

    // Private ctor to disallow creations of objects of this class.
    private WebViewChromiumFactoryProviderSingleton() {}

    public static void create(WebViewChromiumFactoryProvider provider) {
        synchronized (sLock) {
            if (sProvider != null) {
                throw new RuntimeException(
                        "WebViewChromiumFactoryProvider should only be set once!");
            }
            sProvider = provider;
        }
    }

    public static WebViewChromiumFactoryProvider getProvider() {
        synchronized (sLock) {
            if (sProvider == null) {
                throw new RuntimeException("WebViewChromiumFactoryProvider has not been set!");
            }
            return sProvider;
        }
    }
}
