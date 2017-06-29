package com.android.support_library_prototype;

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.AbsoluteLayout;

import org.chromium.android_webview_glue.GlueWebViewClient;
import org.chromium.android_webview_glue.GlueWebViewProvider;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Map;

public class SupportLibWebView extends AbsoluteLayout {
    private ClassLoader mWebViewClassLoader;
    private Object mProvider; // GlueWebViewProvider

    public SupportLibWebView(Context context) {
        super(context);

        mWebViewClassLoader = getWebViewClassLoader();
        mProvider = createGlueWebViewProvider();
    }

    public void loadUrl(String url) {
        callLoadUrl(url);
    }

    private void callLoadUrl(String url) {
        try {
            Method loadUrlMethod = mProvider.getClass().getDeclaredMethod("loadUrl", String.class);
            loadUrlMethod.invoke(mProvider, url);
        } catch (NoSuchMethodException | IllegalAccessException | InvocationTargetException e) {
            throw new RuntimeException(e);
        }
    }

    public void setWebViewClient(WebViewClient webviewClient) {
        // TODO there must be some way of automating the creation of the middle-man here..
        callSetWebViewClient(createGlueWebViewClient(webviewClient));
    }

    /**
     * Call into glue-layer to set GlueWebViewClient.
     * @param glueWebViewClient
     */
    private void callSetWebViewClient(Object glueWebViewClient) {
        try {
            Class glueWebViewClientClass =
                    Class.forName(GlueWebViewClient.class.getName(), false, mWebViewClassLoader);
            Method setWebViewClientMethod = mProvider.getClass().getDeclaredMethod(
                    "setWebViewClient", glueWebViewClientClass);
            setWebViewClientMethod.invoke(mProvider, glueWebViewClient);
        } catch (ClassNotFoundException | NoSuchMethodException | IllegalAccessException
                | InvocationTargetException e) {
            throw new RuntimeException(e);
        }
    }

    private Object createGlueWebViewProvider() {
        try {
            Class glueWebViewProviderClass =
                    Class.forName(GlueWebViewProvider.class.getName(), false, mWebViewClassLoader);
            Object glueWebViewProvider = glueWebViewProviderClass.newInstance();
            Method initMethod = glueWebViewProviderClass.getDeclaredMethod(
                    "init", ViewGroup.class, Context.class);
            initMethod.invoke(glueWebViewProvider, this, getContext());
            return glueWebViewProvider;
        } catch (InstantiationException | IllegalAccessException | NoSuchMethodException
                | ClassNotFoundException | InvocationTargetException e) {
            throw new RuntimeException(e);
        }
    }

    private static ClassLoader getWebViewClassLoader() {
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

    /**
     * Use reflection to create an object implementing GlueWebViewClient by delegating calls to
     * {@param webviewClient}.
     * @param webviewClient
     * @return
     */
    private Object createGlueWebViewClient(final WebViewClient webviewClient) {
        InvocationHandler invocationHandler = new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
                if (method.getName().equals("onPageFinished")) {
                    Log.e("InvocationHandler",
                            "in invoke, for onPageFinished, with url " + objects[0]);
                    // TODO will have to ensure the objects passed into the method call are of
                    // correct types.
                    webviewClient.onPageFinished((String) objects[0]);
                    return null;
                } else if (method.getName().equals("onReceivedError")) {
                    Log.e("InvocationHandler",
                            "in invoke, for onReceivedError, with request " + objects[0]);
                    webviewClient.onReceivedError(
                            new WebResourceRequestImpl(mWebViewClassLoader, objects[0]),
                            new WebResourceErrorImpl(objects[1]));
                    return null;
                }
                Log.e("InvocationHandler",
                        "in invoke, not for onPageFinished? " + method.toString());
                return null;
            }
        };
        try {
            Class glueWebViewClientClass =
                    Class.forName(GlueWebViewClient.class.getName(), false, mWebViewClassLoader);
            return Proxy.newProxyInstance(
                    mWebViewClassLoader, new Class[] {glueWebViewClientClass}, invocationHandler);
        } catch (ClassNotFoundException e) {
            throw new RuntimeException(e);
        }
    }

    // TODO implement...
    private static class WebResourceErrorImpl implements WebResourceError {
        private final Object mError;

        WebResourceErrorImpl(Object glueError) {
            mError = glueError;
        }

        @Override
        public CharSequence getDescription() {
            return null;
        }

        @Override
        public int getErrorCode() {
            return 0;
        }
    }

    private static class WebResourceRequestImpl implements WebResourceRequest {
        private final ClassLoader mClassLoader;
        private final Object mGlueResponse;

        WebResourceRequestImpl(ClassLoader glueClassloader, Object glueResponse) {
            mClassLoader = glueClassloader;
            mGlueResponse = glueResponse;
        }

        // Invoke a method without any parameters.
        private Object invokeMethod(String methodName) {
            try {
                Class glueWebResourceRequestClass = Class.forName(
                        org.chromium.android_webview_glue.WebResourceRequest.class.getName(), false,
                        mClassLoader);
                Method method = glueWebResourceRequestClass.getDeclaredMethod(methodName);
                return method.invoke(mGlueResponse);
            } catch (ClassNotFoundException | NoSuchMethodException | IllegalAccessException
                    | InvocationTargetException e) {
                throw new RuntimeException(e);
            }
        }

        @Override
        public Uri getUrl() {
            return (Uri) invokeMethod("getUrl");
        }

        @Override
        public boolean isForMainFrame() {
            return (Boolean) invokeMethod("isForMainFrame");
        }

        @Override
        public boolean isRedirect() {
            return (Boolean) invokeMethod("isRedirect");
        }

        @Override
        public boolean hasGesture() {
            return (Boolean) invokeMethod("hasGesture");
        }

        @Override
        public String getMethod() {
            return (String) invokeMethod("getMethod");
        }

        @Override
        public Map<String, String> getRequestHeaders() {
            return (Map<String, String>) invokeMethod("getRequestHeaders");
        }
    }
}
