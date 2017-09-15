package com.android.webviewsupportlib;

import android.util.Log;
import android.webkit.WebView;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

/**
 * Created by gsennton on 8/10/17.
 */
// TODO the WebViewProviderFactory should be stored in here - WebView.java should then fetch the
// static instance of the provider from here.
public class WebViewGlueCommunicator {
    private final static String TAG = WebViewGlueCommunicator.class.getSimpleName();
    private static ClassLoader sWebViewClassLoader;
    private static WebViewProviderFactory sProvider; // GlueWebViewProvider

    private static final String GLUE_FACTORY_PROVIDER_FETCHER_CLASS =
            "org.chromium.android_webview_glue.GlueFactoryProviderFetcher";

    public static WebViewProviderFactory getFactory() {
        ensureWebViewLoaded();
        return sProvider;
    }

    private static void ensureWebViewLoaded() {
        if (sProvider == null) {
            sWebViewClassLoader = getWebViewClassLoader();
            Log.e(TAG, "calling createGlueWebViewProvider()", new Throwable());
            sProvider = createGlueProviderFactory();
        }
    }

    private static InvocationHandler fetchGlueProviderFactoryImpl() {
        try {
            Class glueFactoryProviderFetcherClass = Class.forName(
                    GLUE_FACTORY_PROVIDER_FETCHER_CLASS, false, getWebViewClassLoader());
            Method createProviderFactoryMethod = glueFactoryProviderFetcherClass.getDeclaredMethod(
                    "createGlueWebViewProviderFactory");
            return (InvocationHandler) createProviderFactoryMethod.invoke(null);
        } catch (IllegalAccessException | InvocationTargetException | ClassNotFoundException
                | NoSuchMethodException e) {
            throw new RuntimeException(e);
        }
    }

    private static WebViewProviderFactory createGlueProviderFactory() {
        InvocationHandler invocationHandler = fetchGlueProviderFactoryImpl();
        return (WebViewProviderFactory) Proxy.newProxyInstance(
                WebViewGlueCommunicator.class.getClassLoader(),
                new Class[] {WebViewProviderFactory.class}, invocationHandler);
    }

    /**
     *
     * @param clazz
     * @param invocationHandler
     */
    public static <T> T castToSuppLibClass(Class<T> clazz, InvocationHandler invocationHandler) {
        return clazz.cast(Proxy.newProxyInstance(WebViewGlueCommunicator.class.getClassLoader(),
                new Class[] {clazz}, invocationHandler));
    }

    // Load the WebView code from the WebView APK and return the classloader containing that code.
    public static ClassLoader getWebViewClassLoader() {
        return getWebViewProviderFactory().getClass().getClassLoader();
    }

    private static Object getWebViewProviderFactory() {
        try {
            Method getFactoryMethod = WebView.class.getDeclaredMethod("getFactory");
            getFactoryMethod.setAccessible(true);
            return getFactoryMethod.invoke(null);
        } catch (NoSuchMethodException e) {
            throw new RuntimeException(e);
        } catch (InvocationTargetException e) {
            throw new RuntimeException(e);
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        }
    }

    public static Method dupeMethod(Method method)
            throws ClassNotFoundException, NoSuchMethodException {
        Class declaringClass = Class.forName(method.getDeclaringClass().getName());
        Class[] otherSideParameterClasses = method.getParameterTypes();
        Class[] parameterClasses = new Class[otherSideParameterClasses.length];
        for (int n = 0; n < parameterClasses.length; n++) {
            parameterClasses[n] = Class.forName(otherSideParameterClasses[n].getName());
        }
        return declaringClass.getDeclaredMethod(method.getName(), parameterClasses);
    }
}
