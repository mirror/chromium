// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.android_webview_glue;

import org.chromium.base.Log;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;

// TODO returns an invocation handler that implements GlueWebViewProviderFactory.
public class GlueFactoryProviderFetcher {
    private final static String TAG = GlueFactoryProviderFetcher.class.getSimpleName();

    // GlueFactoryProviderFetcher() {
    //}
    // TODO parameters
    public static InvocationHandler createGlueWebViewProviderFactory() {
        final GlueWebViewChromiumProviderFactory factory =
                GlueWebViewChromiumProviderFactory.create();
        return new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
                // Log.e(TAG, "in InvocationHandler.invoke for " + o + " method: " + method);
                // TODO don't forget to hook this up with a Proxy on the support library
                // side - having this implement the supp-lib version of the WebViewFactory
                // interface (i.e. the one in that ClassLoader).
                Class declaringClass = Class.forName(method.getDeclaringClass().getName());
                Method myMethod = declaringClass.getDeclaredMethod(
                        method.getName(), method.getParameterTypes());
                return myMethod.invoke(factory, objects);
            }
        };
    }

    /**
     * Returns same method but for the current classloader.
     */
    public static Method dupeMethod(Method method)
            throws ClassNotFoundException, NoSuchMethodException {
        Class declaringClass = Class.forName(method.getDeclaringClass().getName());
        Method ret = declaringClass.getDeclaredMethod(method.getName(),
                // TODO this probably just works because the parameter types are shared between both
                // classloaders, if they are not we should need to dupe the parameter classes as
                // well:
                // Class[] oldParameterClasses = method.getParameterTypes();
                // Class[] newParameterClasses = new Class[oldParameterClasses.length];
                // for (int n = 0; n < oldParameterClasses.length; n++) {
                //    newParameterClasses[n] = Class.forName(oldParameterClasses[n].getName());
                //}
                method.getParameterTypes());
        return ret;
    }
}
