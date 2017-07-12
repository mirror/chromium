// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.android_webview_glue;

import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Build;
import android.os.Handler;
import android.util.Log;
import android.view.DragEvent;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStructure;
import android.view.accessibility.AccessibilityNodeProvider;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

import com.android.webview.chromium.DrawGLFunctor;
import com.android.webview.chromium.WebViewDelegateFactory.WebViewDelegate;
import com.android.webview.chromium.WebViewDelegateFactory;

import org.chromium.android_webview.AwBrowserState;
import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.AwContentsStatics;
import org.chromium.android_webview.AwSettings;
import org.chromium.components.autofill.AutofillProvider;

// To update glue-layer in support library:
// cp ./out-gn/Debug/lib.java/android_webview/support_library_glue/support_lib_prototype_java.jar
// /usr/local/google/code/AndroidStudioProjects/AAAAAsdasdasdas/app/libs/

// This is essentially WebViewChromium (a WebViewProvider).
public class GlueWebViewProvider {
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

    private WebViewDelegate mWebViewDelegate;

    // protected WebViewChromiumFactoryProvider mFactory;

    // private final boolean mShouldDisableThreadChecking;

    public GlueWebViewProvider() {
        mAppTargetSdkVersion = Build.VERSION_CODES.LOLLIPOP;
    }

    // L-version of init
    public void init(ViewGroup containerView, Context context) {
        init(containerView, context, null);
    }

    public void init(
            ViewGroup containerView, Context context, android.webkit.WebViewDelegate delegate) {
        Log.e(TAG, "in init(..)");
        mContainerView = containerView;
        mContext = context;
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP) {
            mWebViewDelegate = WebViewDelegateFactory.createProxyDelegate(delegate);
        } else {
            mWebViewDelegate = WebViewDelegateFactory.createApi21CompatibilityDelegate();
        }

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
            // TODO the WebView delegate should really be stored in the WebViewFactoryProvider since
            // it's shared between WebViews, but right now we just store it in the WebViewProvider
            // as a hack.
            return new DrawGLFunctor(context, mWebViewDelegate);
        }
    }

    public void loadUrl(final String url) {
        // [Prototype] Currently just assuming this is run on the UI thread.
        Log.e(TAG, "in loadUrl with url " + url);
        mAwContents.loadUrl(url);
    }

    public void setWebViewClient(GlueWebViewClient glueWebViewClient) {
        Log.e(TAG, "in setWebViewClient");
        mContentsClientAdapter.setWebViewClient(glueWebViewClient);
    }

    public void onDraw(final Canvas canvas) {
        Log.e(TAG, "in onDraw");
        mAwContents.onDraw(canvas);
    }

    public void onAttachedToWindow() {
        Log.e(TAG, "in onAttachedToWindow");
        mAwContents.onAttachedToWindow();
    }

    public void onSizeChanged(final int w, final int h, final int ow, final int oh) {
        Log.e(TAG, String.format("in onSizeChanged, (w,h,ow,oh) (%d,%d,%d,%d)", w, h, ow, oh));
        mAwContents.onSizeChanged(w, h, ow, oh);
    }

    // -------------------------------------------------------------
    // Overridden View-methods
    // -------------------------------------------------------------

    public void setLayoutParams(ViewGroup.LayoutParams params) {
        // TODO private-super . setLayoutParams?
        mAwContents.setLayoutParams(params);
    }

    public void setOverScrollMode(int mode) {
        // This method may be called in the constructor chain, before the WebView provider is
        // created.
        mAwContents.setOverScrollMode(mode);
    }

    public void setScrollBarStyle(int style) {
        mAwContents.setScrollBarStyle(style);
    }

    public int computeHorizontalScrollRange() {
        return mAwContents.computeHorizontalScrollRange();
    }

    public int computeHorizontalScrollOffset() {
        return mAwContents.computeHorizontalScrollOffset();
    }

    public int computeVerticalScrollRange() {
        return mAwContents.computeVerticalScrollRange();
    }

    public int computeVerticalScrollOffset() {
        return mAwContents.computeVerticalScrollOffset();
    }

    public int computeVerticalScrollExtent() {
        return mAwContents.computeVerticalScrollExtent();
    }

    public void computeScroll() {
        mAwContents.computeScroll();
    }

    public boolean onHoverEvent(MotionEvent event) {
        return mAwContents.onHoverEvent(event);
    }

    public boolean onTouchEvent(MotionEvent event) {
        return mAwContents.onTouchEvent(event);
    }

    public boolean onGenericMotionEvent(MotionEvent event) {
        return mAwContents.onGenericMotionEvent(event);
    }

    public boolean onTrackballEvent(MotionEvent event) {
        return false;
    }

    public boolean onKeyDown(int keyCode, KeyEvent event) {
        return false;
    }

    public boolean onKeyUp(int keyCode, KeyEvent event) {
        return mAwContents.onKeyUp(keyCode, event);
    }

    public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event) {
        return false;
    }

    public AccessibilityNodeProvider getAccessibilityNodeProvider() {
        return mAwContents.getAccessibilityNodeProvider();
    }

    public CharSequence getAccessibilityClassName() {
        // TODO ok? Shouldn't this be the full class name of WebView?
        return "android.webkit.WebView";
    }

    public void onProvideVirtualStructure(ViewStructure structure) {
        mAwContents.onProvideVirtualStructure(structure);
    }

    // public void onProvideAutofillVirtualStructure(ViewStructure structure, int flags) {
    //    mAwContents.onProvideAutofillVirtualStructure(structure, flags);
    //}

    // public void autofill(SparseArray<AutofillValue>values) {
    //    mAwContents.autofill(values);
    //}

    ///** @hide */
    // public void onInitializeAccessibilityNodeInfoInternal(AccessibilityNodeInfo info) {
    //    super.onInitializeAccessibilityNodeInfoInternal(info);
    //    mAwContents.onInitializeAccessibilityNodeInfo(info);
    //}

    ///** @hide */
    //@Override
    // public void onInitializeAccessibilityEventInternal(AccessibilityEvent event) {
    //    super.onInitializeAccessibilityEventInternal(event);
    //    mAwContents.onInitializeAccessibilityEvent(event);
    //}

    ///** @hide */
    //@Override
    // public boolean performAccessibilityActionInternal(int action, Bundle arguments) {
    //    return mAwContents.performAccessibilityAction(action, arguments);
    //}

    ///** @hide */
    //@Override
    // protected void onDrawVerticalScrollBar(Canvas canvas, Drawable scrollBar,
    //        int l, int t, int r, int b) {
    //    mAwContents.onDrawVerticalScrollBar(canvas, scrollBar, l, t, r, b);
    //}

    public void onOverScrolled(int scrollX, int scrollY, boolean clampedX, boolean clampedY) {
        mAwContents.onContainerViewOverScrolled(scrollX, scrollY, clampedX, clampedY);
    }

    public void onWindowVisibilityChanged(int visibility) {
        mAwContents.onWindowVisibilityChanged(visibility);
    }

    // protected void onDraw(Canvas canvas) {
    //    mAwContents.onDraw(canvas);
    //}

    // public boolean performLongClick() {
    //    return mAwContents.performLongClick();
    //}

    public void onConfigurationChanged(Configuration newConfig) {
        mAwContents.onConfigurationChanged(newConfig);
    }

    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return mAwContents.onCreateInputConnection(outAttrs);
    }

    public boolean onDragEvent(DragEvent event) {
        return mAwContents.onDragEvent(event);
    }

    public void onVisibilityChanged(View changedView, int visibility) {
        // This method may be called in the constructor chain, before the WebView provider is
        // created.
        mAwContents.onVisibilityChanged(changedView, visibility);
    }

    public void onWindowFocusChanged(boolean hasWindowFocus) {
        mAwContents.onWindowFocusChanged(hasWindowFocus);
    }

    public void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
        mAwContents.onFocusChanged(focused, direction, previouslyFocusedRect);
    }

    ///** @hide */
    //@Override
    // protected boolean setFrame(int left, int top, int right, int bottom) {
    //    return mAwContents.setFrame(left, top, right, bottom);
    //}

    //@Override
    // protected void onSizeChanged(int w, int h, int ow, int oh) {
    //    super.onSizeChanged(w, h, ow, oh);
    //    mAwContents.onSizeChanged(w, h, ow, oh);
    //}

    public void onScrollChanged(int l, int t, int oldl, int oldt) {
        mAwContents.onContainerViewScrollChanged(l, t, oldl, oldt);
    }

    public boolean dispatchKeyEvent(KeyEvent event) {
        return mAwContents.dispatchKeyEvent(event);
    }

    public boolean requestFocus(int direction, Rect previouslyFocusedRect) {
        mAwContents.requestFocus();
        // this return value is ignored.
        return true;
    }

    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        mAwContents.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    public boolean requestChildRectangleOnScreen(View child, Rect rect, boolean immediate) {
        return mAwContents.requestChildRectangleOnScreen(child, rect, immediate);
    }

    public void setBackgroundColor(int color) {
        mAwContents.setBackgroundColor(color);
    }

    public void setLayerType(int layerType, Paint paint) {
        mAwContents.setLayerType(layerType, paint);
    }

    public void preDispatchDraw(Canvas canvas) {}

    public void onStartTemporaryDetach() {
        mAwContents.onStartTemporaryDetach();
    }

    public void onFinishTemporaryDetach() {
        mAwContents.onFinishTemporaryDetach();
    }

    public Handler getHandler(Handler handler) {
        return handler;
    }

    public View findFocus(View focus) {
        return focus;
    }
}
