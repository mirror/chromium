// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.android_webview_glue;

import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Build;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ViewGroup;

import com.android.webviewsupportlib.WebView;
import com.android.webviewsupportlib.WebViewClientInterface;
import com.android.webviewsupportlib.WebViewProvider;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

import org.chromium.android_webview.AwBrowserState;
import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.AwContentsStatics;
import org.chromium.android_webview.AwSettings;
import org.chromium.components.autofill.AutofillProvider;

// To update glue-layer in support library:
// cp ./out-gn/Debug/lib.java/android_webview/support_library_glue/support_lib_prototype_java.jar
// /usr/local/google/code/AndroidStudioProjects/AAAAAsdasdasdas/app/libs/

// This is essentially WebViewChromium (a WebViewProvider).
public class GlueWebViewProvider extends GlueWebViewProviderImplementer {
    private static final String TAG = GlueWebViewProvider.class.getSimpleName();
    // private GlueWebViewFactoryProvider mFactory;

    // The WebView that this WebViewChromium is the provider for.
    // WebView mWebView;
    //// Lets us access protected View-derived methods on the WebView instance we're backing.
    // WebView.PrivateAccess mWebViewPrivate;
    // The client adapter class.
    // private WebViewContentsClientAdapter mContentsClientAdapter;
    private GlueWebViewContentsClientAdapter mContentsClientAdapter;
    // The wrapped Context.
    private Context mContext;

    // Adding this instead of mWebView (works for some uses).
    private ViewGroup mContainerView;

    // Variables for functionality provided by this adapter ---------------------------------------
    // private ContentSettingsAdapter mWebSettings;
    private AwSettings mAwSettings;
    // The WebView wrapper for ContentViewCore and required browser compontents.
    AwContents mAwContents;
    // Non-null if this webview is using the GL accelerated draw path.
    // private DrawGLFunctor mGLfunctor;

    // private final WebView.HitTestResult mHitTestResult;

    private final int mAppTargetSdkVersion;

    // protected WebViewChromiumFactoryProvider mFactory;

    // private final boolean mShouldDisableThreadChecking;
    public GlueWebViewProvider(
            GlueWebViewChromiumProviderFactory factory, ViewGroup containerView, Context context) {
        mAppTargetSdkVersion = Build.VERSION_CODES.LOLLIPOP;
        init(containerView, context);
    }

    public GlueWebViewProvider(GlueWebViewChromiumProviderFactory factory, WebView webview,
            WebView.PrivateAccess webviewPrivate) {
        mAppTargetSdkVersion = Build.VERSION_CODES.LOLLIPOP;
    }

    // TODO this init method is messed up... we should pass the container view to the ctor..
    public void init(ViewGroup containerView, Context context) {
        Log.e(TAG, "in init(..)");
        mContainerView = containerView;
        mContext = context;

        final boolean isAccessFromFileURLsGrantedByDefault =
                mAppTargetSdkVersion < Build.VERSION_CODES.JELLY_BEAN;
        final boolean areLegacyQuirksEnabled = mAppTargetSdkVersion < Build.VERSION_CODES.KITKAT;
        final boolean allowEmptyDocumentPersistence = mAppTargetSdkVersion <= Build.VERSION_CODES.M;
        final boolean allowGeolocationOnInsecureOrigins =
                mAppTargetSdkVersion <= Build.VERSION_CODES.M;

        // https://crbug.com/698752
        final boolean doNotUpdateSelectionOnMutatingSelectionRange =
                mAppTargetSdkVersion <= Build.VERSION_CODES.M;

        mContentsClientAdapter = new GlueWebViewContentsClientAdapter(mContext);
        // mContentsClientAdapter = mFactory.createWebViewContentsClientAdapter(mWebView, mContext);
        mAwSettings = new AwSettings(mContext, isAccessFromFileURLsGrantedByDefault,
                areLegacyQuirksEnabled, allowEmptyDocumentPersistence,
                allowGeolocationOnInsecureOrigins, doNotUpdateSelectionOnMutatingSelectionRange);
        // mWebSettings = new ContentSettingsAdapter(new AwSettings(mContext,
        //        isAccessFromFileURLsGrantedByDefault, areLegacyQuirksEnabled,
        //        allowEmptyDocumentPersistence, allowGeolocationOnInsecureOrigins,
        //        doNotUpdateSelectionOnMutatingSelectionRange));

        initForReal();
    }

    private void initForReal() {
        AwContentsStatics.setRecordFullDocument(
                mAppTargetSdkVersion < Build.VERSION_CODES.LOLLIPOP);

        mAwContents = new AwContents(AwBrowserState.getBrowserContextOnUiThread(), mContainerView,
                mContext, new InternalAccessAdapter(), new WebViewNativeDrawGLFunctorFactory(),
                mContentsClientAdapter, mAwSettings /*mWebSettings.getAwSettings()*/,
                new AwContents.DependencyFactory() {
                    @Override
                    public AutofillProvider createAutofillProvider(
                            Context context, ViewGroup containerView) {
                        return null;
                    }
                });

        if (mAppTargetSdkVersion >= Build.VERSION_CODES.KITKAT) {
            // On KK and above, favicons are automatically downloaded as the method
            // old apps use to enable that behavior is deprecated.
            AwContents.setShouldDownloadFavicons();
        }

        if (mAppTargetSdkVersion < Build.VERSION_CODES.LOLLIPOP) {
            // Prior to Lollipop, JavaScript objects injected via addJavascriptInterface
            // were not inspectable.
            mAwContents.disableJavascriptInterfacesInspection();
        }

        mAwContents.setLayerType(mContainerView.getLayerType(), null);
    }

    private class InternalAccessAdapter implements AwContents.InternalAccessDelegate {
        @Override
        public boolean super_onKeyUp(int arg0, KeyEvent arg1) {
            // Intentional no-op
            return false;
        }

        @Override
        public boolean super_dispatchKeyEvent(KeyEvent event) {
            return false;
            // return mWebViewPrivate.super_dispatchKeyEvent(event);
        }

        @Override
        public boolean super_onGenericMotionEvent(MotionEvent arg0) {
            return false;
            // return mWebViewPrivate.super_onGenericMotionEvent(arg0);
        }

        @Override
        public void super_onConfigurationChanged(Configuration arg0) {
            // Intentional no-op
        }

        @Override
        public int super_getScrollBarStyle() {
            return 0;
            // return mWebViewPrivate.super_getScrollBarStyle();
        }

        @Override
        public void super_startActivityForResult(Intent intent, int requestCode) {
            // if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            //    mWebViewPrivate.super_startActivityForResult(intent, requestCode);
            //} else {
            //    try {
            //        Method startActivityForResultMethod =
            //                View.class.getMethod("startActivityForResult", Intent.class,
            //                int.class);
            //        startActivityForResultMethod.invoke(mContainerView, intent, requestCode);
            //    } catch (Exception e) {
            //        throw new RuntimeException("Invalid reflection", e);
            //    }
            //}
        }

        @Override
        public boolean awakenScrollBars() {
            // mWebViewPrivate.awakenScrollBars(0);
            // TODO: modify the WebView.PrivateAccess to provide a return value.
            return true;
        }

        @Override
        public boolean super_awakenScrollBars(int arg0, boolean arg1) {
            return false;
        }

        @Override
        public void onScrollChanged(int l, int t, int oldl, int oldt) {
            // Intentional no-op.
            // Chromium calls this directly to trigger accessibility events. That isn't needed
            // for WebView since super_scrollTo invokes onScrollChanged for us.
        }

        @Override
        public void overScrollBy(int deltaX, int deltaY, int scrollX, int scrollY, int scrollRangeX,
                int scrollRangeY, int maxOverScrollX, int maxOverScrollY, boolean isTouchEvent) {
            // mWebViewPrivate.overScrollBy(deltaX, deltaY, scrollX, scrollY, scrollRangeX,
            //        scrollRangeY, maxOverScrollX, maxOverScrollY, isTouchEvent);
        }

        @Override
        public void super_scrollTo(int scrollX, int scrollY) {
            // mWebViewPrivate.super_scrollTo(scrollX, scrollY);
        }

        @Override
        public void setMeasuredDimension(int measuredWidth, int measuredHeight) {
            // mWebViewPrivate.setMeasuredDimension(measuredWidth, measuredHeight);
        }

        // @Override
        public boolean super_onHoverEvent(MotionEvent event) {
            return false;
            // return mWebViewPrivate.super_onHoverEvent(event);
        }
    }

    // AwContents.NativeDrawGLFunctorFactory implementation ----------------------------------
    private class WebViewNativeDrawGLFunctorFactory
            implements AwContents.NativeDrawGLFunctorFactory {
        @Override
        public AwContents.NativeDrawGLFunctor createFunctor(long context) {
            // return new DrawGLFunctor(context, mFactory.getWebViewDelegate());
            return new GlueDrawGLFunctor();
        }
    }

    public void loadUrl(final String url) {
        // [Prototype] Currently just assuming this is run on the UI thread.
        Log.e(TAG, "in loadUrl with url " + url);
        mAwContents.loadUrl(url);
    }

    @Override
    public void setWebViewClient(InvocationHandler invocationHandler) {
        Log.e(TAG, "in setWebViewClient");
        WebViewClientInterface webviewClient = (WebViewClientInterface) Proxy.newProxyInstance(
                GlueWebViewProvider.class.getClassLoader(),
                new Class[] {WebViewClientInterface.class}, invocationHandler);
        mContentsClientAdapter.setWebViewClient(webviewClient);
    }

    public InvocationHandler getViewDelegate() {
        return new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
                Method newMethod = GlueFactoryProviderFetcher.dupeMethod(method);
                // TODO note that passing "this" here causes a run-time failure because we are
                // passing the InvocationHandler instead of the GlueWebViewProvider.
                return newMethod.invoke(GlueWebViewProvider.this, objects);
            }
        };
    }
}
