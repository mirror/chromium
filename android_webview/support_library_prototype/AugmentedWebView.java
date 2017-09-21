package com.android.webviewsupportlib;

import android.webkit.WebView;

import java.lang.reflect.Field;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;

/**
 * Created by gsennton on 9/20/17.
 */

public class AugmentedWebView {
    private static final String TAG = AugmentedWebView.class.getSimpleName();
    private WebView mWebView;

    private static WebViewGlueCommunicator sWebViewGlueCommunicator = new WebViewGlueCommunicator();

    private WebViewProvider mProvider;

    public AugmentedWebView(WebView webview) {
        mWebView = webview;
        mProvider = createProvider(fetchProviderFromWebView(webview));
    }

    /**
     * Callback interface supplied to {@link #postVisualStateCallback} for receiving
     * notifications about the visual state.
     */
    public static interface VisualStateCallback {
        void onComplete(long requestId);
    }

    public void postVisualStateCallback(long requestId, final VisualStateCallback callback) {
        //checkThread();
        //mProvider.insertVisualStateCallback(requestId, callback);
        // TODO we will have to determine how to pass objects across the boundary here..
        mProvider.insertVisualStateCallback(requestId, new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
                return WebViewGlueCommunicator.dupeMethod(method).invoke(callback, objects);
            }
        });
    }



    // Private shizzle



    private static WebViewProviderFactory getFactory() {
        return sWebViewGlueCommunicator.getFactory();
    }

    private static android.webkit.WebViewProvider fetchProviderFromWebView(WebView webview) {
        return fetchFieldByReflection("mProvider", webview);
    }

    private static <FieldType> FieldType fetchFieldByReflection(String fieldName, Object owner) {
        try {
            Field mField = WebView.class.getDeclaredField(fieldName);
            mField.setAccessible(true);
            return (FieldType) mField.get(owner);
        } catch (NoSuchFieldException | IllegalAccessException e) {
            throw new RuntimeException(e);
        }
    }

    private static WebViewProvider createProvider(android.webkit.WebViewProvider oldProvider) {
        //checkThread();
        // As this can get called during the base class constructor chain, pass the minimum
        // number of dependencies here; the rest are deferred to init().
        return WebViewGlueCommunicator.castToSuppLibClass(WebViewProvider.class,
                // wvInvoHandler and privateAccessInvoHandler are not actually used - we can't
                // create a Proxy for WebView since WebView is not an interface.
                getFactory().createWebView(oldProvider));
    }
}
