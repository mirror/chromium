// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.android_webview_glue;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

/**
 * Starting point for fetching WebView implementation.
 */
public class GlueFactoryProviderFetcher {
    private final static String TAG = GlueFactoryProviderFetcher.class.getSimpleName();

    public static InvocationHandler createGlueWebViewProviderFactory() {
        final GlueWebViewChromiumProviderFactory factory =
                GlueWebViewChromiumProviderFactory.create();
        return new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
                return dupeMethod(method).invoke(factory, objects);
            }
        };
    }

    /**
     * Returns same method but for the current classloader.
     */
    public static Method dupeMethod(Method method)
            throws ClassNotFoundException, NoSuchMethodException {
        Class declaringClass = Class.forName(method.getDeclaringClass().getName());
        Method ret = declaringClass.getDeclaredMethod(method.getName(), method.getParameterTypes());
        return ret;
    }

    public static <T> T castToSuppLibClass(Class<T> clazz, InvocationHandler invocationHandler) {
        return clazz.cast(
                Proxy.newProxyInstance(GlueWebViewChromiumProviderFactory.class.getClassLoader(),
                        new Class[] {clazz}, invocationHandler));
    }
}
