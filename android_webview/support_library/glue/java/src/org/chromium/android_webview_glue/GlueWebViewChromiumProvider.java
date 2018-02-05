// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.android_webview_glue;

import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.AwSettings;
import org.chromium.android_webview.SharedWebViewProviderState;
import org.chromium.support_lib_boundary.VisualStateCallbackInterface;
import org.chromium.support_lib_boundary.WebViewProvider;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;

/**
 */
public class GlueWebViewChromiumProvider implements WebViewProvider {
    private static final String TAG = GlueWebViewChromiumProvider.class.getSimpleName();

    final AwContents mAwContents;

    final GlueWebSettingsAdapter mWebSettings;

    public GlueWebViewChromiumProvider(AwContents awContents, AwSettings awSettings) {
        mAwContents = awContents;
        mWebSettings = new GlueWebSettingsAdapter(awSettings);
    }

    @Override
    public void insertVisualStateCallback(long requestId, InvocationHandler callbackInvoHandler) {
        final VisualStateCallbackInterface visualStateCallback =
                GlueFactoryProviderFetcher.castToSuppLibClass(
                        VisualStateCallbackInterface.class, callbackInvoHandler);

        mAwContents.insertVisualStateCallback(requestId, new AwContents.VisualStateCallback() {
            @Override
            public void onComplete(long requestId) {
                visualStateCallback.onComplete(requestId);
            }
        });
    }

    @Override
    public InvocationHandler getSettings() {
        // TODO we can probably create a utility method for creating InvocationHandlers..
        return new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
                return GlueFactoryProviderFetcher.dupeMethod(method).invoke(mWebSettings, objects);
            }
        };
    }
}
