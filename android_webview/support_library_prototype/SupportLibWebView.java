package com.android.support_library_prototype;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.net.Uri;
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
import android.webkit.WebView;
import android.widget.AbsoluteLayout;

import org.chromium.android_webview_glue.GlueWebViewClient;
import org.chromium.android_webview_glue.GlueWebViewProvider;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Map;

public class SupportLibWebView extends AbsoluteLayout {
    private final static String TAG = SupportLibWebView.class.getSimpleName();
    private ClassLoader mWebViewClassLoader;
    private Object mProvider; // GlueWebViewProvider

    public SupportLibWebView(Context context) {
        super(context);

        Log.e(TAG, "in ctor");

        ensureWebViewLoaded();

        Log.e(TAG, "in ctor, thread: " + Thread.currentThread().toString());

        //        setWillNotDraw(false);

        //        printWebViewBitmap();
    }

    private void printWebViewBitmap() {
        Handler h = new Handler();
        Log.e(TAG, "posting runnable to current thread");
        h.postDelayed(new Runnable() {
            @Override
            public void run() {
                Log.e(TAG, "in run()");
                Bitmap bm = Bitmap.createBitmap(1600, 900, Bitmap.Config.ARGB_8888);
                // This works! WUHU!
                onDraw(new Canvas(bm));
                printToFile(bm,
                        new File(getContext().getExternalFilesDir(null), "wvsupplib_bm.png")
                                .getAbsolutePath());
            }
        }, 5000);
    }

    private static void printToFile(Bitmap bm, String fileName) {
        Log.e(TAG, "printing to file " + fileName);
        FileOutputStream out = null;
        try {
            out = new FileOutputStream(fileName);
            bm.compress(Bitmap.CompressFormat.PNG, 100, out);
        } catch (FileNotFoundException e) {
            throw new RuntimeException(e);
        } finally {
            try {
                if (out != null) out.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private void ensureWebViewLoaded() {
        if (mProvider == null) {
            mWebViewClassLoader = getWebViewClassLoader();
            Log.e(TAG, "calling createGlueWebViewProvider()", new Throwable());
            mProvider = createGlueWebViewProvider();
        }
    }

    // TODO I might have overestimated the need to explicitly pass class parameters - maybe null is
    // never passed from the framework?
    private Object callWebViewMethod(String methodName, Object... args) {
        try {
            Method method = null;
            switch (args.length) {
                case 0:
                    method = mProvider.getClass().getDeclaredMethod(methodName);
                    return method.invoke(mProvider);
                case 2:
                    method = mProvider.getClass().getDeclaredMethod(methodName, (Class) args[0]);
                    return method.invoke(mProvider, args[1]);
                case 4:
                    method = mProvider.getClass().getDeclaredMethod(
                            methodName, (Class) args[0], (Class) args[1]);
                    return method.invoke(mProvider, args[2], args[3]);
                case 6:
                    method = mProvider.getClass().getDeclaredMethod(
                            methodName, (Class) args[0], (Class) args[1], (Class) args[2]);
                    return method.invoke(mProvider, args[3], args[4], args[5]);
                case 8:
                    method = mProvider.getClass().getDeclaredMethod(methodName, (Class) args[0],
                            (Class) args[1], (Class) args[2], (Class) args[3]);
                    return method.invoke(mProvider, args[4], args[5], args[6], args[7]);
                default:
                    throw new RuntimeException("Wrong number of args :D " + args.length);
            }
        } catch (NoSuchMethodException | IllegalAccessException | InvocationTargetException e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    protected void onSizeChanged(int w, int h, int ow, int oh) {
        super.onSizeChanged(w, h, ow, oh);
        callOnSizeChanged(w, h, ow, oh);
        Log.e(TAG, String.format("in onSizeChanged, (w,h,ow,oh) (%d,%d,%d,%d)", w, h, ow, oh));
    }

    private void callOnSizeChanged(int w, int h, int ow, int oh) {
        try {
            Method onSizeChangedMethod = mProvider.getClass().getDeclaredMethod(
                    "onSizeChanged", int.class, int.class, int.class, int.class);
            onSizeChangedMethod.invoke(mProvider, w, h, ow, oh);
        } catch (NoSuchMethodException | IllegalAccessException | InvocationTargetException e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public void onDraw(Canvas canvas) {
        Log.e(TAG, "in onDraw");
        callOnDraw(canvas);
    }

    private void callOnDraw(Canvas canvas) {
        try {
            Method onDrawMethod = mProvider.getClass().getDeclaredMethod("onDraw", Canvas.class);
            onDrawMethod.invoke(mProvider, canvas);
        } catch (NoSuchMethodException | IllegalAccessException | InvocationTargetException e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public void onAttachedToWindow() {
        Log.e(TAG, "in onAttachedToWindow");
        callOnAttachedToWindow();
    }

    private void callOnAttachedToWindow() {
        try {
            Method onDrawMethod = mProvider.getClass().getDeclaredMethod("onAttachedToWindow");
            onDrawMethod.invoke(mProvider);
        } catch (NoSuchMethodException | IllegalAccessException | InvocationTargetException e) {
            throw new RuntimeException(e);
        }
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
            // TODO we should be building against the system-sdk, and with that sdk we can just pass
            // WebViewDelegate.class here normally (on L MR1 and above). Though, we should really
            // pass the WebViewDelegate to the WebViewProviderFactory ctor.
            Log.e(TAG, "BuildConfig.VERSION_CODE: " + BuildConfig.VERSION_CODE);
            if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP) { // L MR1, and above
                Log.e(TAG, "gonna call GlueWebViewProvider.init(3 params)");
                Class webviewDelegateClass = Class.forName("android.webkit.WebViewDelegate");
                Method initMethod = glueWebViewProviderClass.getDeclaredMethod(
                        "init", ViewGroup.class, Context.class, webviewDelegateClass);
                Constructor webviewDelegateCtor = webviewDelegateClass.getDeclaredConstructor();
                webviewDelegateCtor.setAccessible(true);
                Object webviewDelegate = webviewDelegateCtor.newInstance();
                initMethod.invoke(glueWebViewProvider, this, getContext(), webviewDelegate);
            } else { // Lollipop
                Log.e(TAG, "gonna call GlueWebViewProvider.init(2 params)");
                Method initMethod = glueWebViewProviderClass.getDeclaredMethod(
                        "init", ViewGroup.class, Context.class);
                initMethod.invoke(glueWebViewProvider, this, getContext());
            }
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

    //    Automatically generated method calls

    //    @Override
    //    protected void onAttachedToWindow() {
    //        super.onAttachedToWindow();
    //        callWebViewMethod("onAttachedToWindow");
    //    }
    //
    //    /** @hide */
    //    @Override
    //    protected void onDetachedFromWindowInternal() {
    //        callWebViewMethod("onDetachedFromWindow");
    //        super.onDetachedFromWindowInternal();
    //    }
    //
    //    /** @hide */
    //    @Override
    //    public void onMovedToDisplay(int displayId, Configuration config) {
    //        callWebViewMethod("onMovedToDisplay", displayId, config);
    //    }

    @Override
    public void setLayoutParams(ViewGroup.LayoutParams params) {
        Log.e(TAG, "in setLayoutParams " + params.width + " " + params.height);
        // TODO normally the glue-layer would call both AwContents and WebView.super.
        callWebViewMethod("setLayoutParams", ViewGroup.LayoutParams.class, params);
        super.setLayoutParams(params);
    }

    @Override
    public void setOverScrollMode(int mode) {
        super.setOverScrollMode(mode);
        //        ensureWebViewLoaded();
        Log.e(TAG,
                "in setOverScrollMode " + mode + " thread: " + Thread.currentThread().toString());
        // This method may be called in the constructor chain, before the WebView provider is
        // created.
        //        ensureProviderCreated();
        // TODO setOverScrollMode is called in the View ctor. If we initiate the WebViewProvider at
        // that state we are creating AwContents before the our ViewGroup ctor finished running. The
        // ViewGroup ctor calls View.setWillNotDraw(true), while AwContents initialization calls
        // View.setWillNotDraw(false). Thus, if AwContents is initialized before ViewGroup.this the
        // last View.setWillNotDraw() will be passing 'true', thus declaring that this class does
        // not draw its own content.
        if (mProvider != null) {
            callWebViewMethod("setOverScrollMode", int.class, mode);
        }
    }

    @Override
    public void setScrollBarStyle(int style) {
        callWebViewMethod("setScrollBarStyle", int.class, style);
        super.setScrollBarStyle(style);
    }

    @Override
    protected int computeHorizontalScrollRange() {
        return (int) callWebViewMethod("computeHorizontalScrollRange");
    }

    @Override
    protected int computeHorizontalScrollOffset() {
        return (int) callWebViewMethod("computeHorizontalScrollOffset");
    }

    @Override
    protected int computeVerticalScrollRange() {
        return (int) callWebViewMethod("computeVerticalScrollRange");
    }

    @Override
    protected int computeVerticalScrollOffset() {
        return (int) callWebViewMethod("computeVerticalScrollOffset");
    }

    @Override
    protected int computeVerticalScrollExtent() {
        return (int) callWebViewMethod("computeVerticalScrollExtent");
    }

    @Override
    public void computeScroll() {
        callWebViewMethod("computeScroll");
    }

    @Override
    public boolean onHoverEvent(MotionEvent event) {
        return (boolean) callWebViewMethod("onHoverEvent", MotionEvent.class, event);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        return (boolean) callWebViewMethod("onTouchEvent", MotionEvent.class, event);
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        return (boolean) callWebViewMethod("onGenericMotionEvent", MotionEvent.class, event);
    }

    @Override
    public boolean onTrackballEvent(MotionEvent event) {
        return (boolean) callWebViewMethod("onTrackballEvent", MotionEvent.class, event);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        return (boolean) callWebViewMethod("onKeyDown", int.class, KeyEvent.class, keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        return (boolean) callWebViewMethod("onKeyUp", int.class, KeyEvent.class, keyCode, event);
    }

    @Override
    public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event) {
        return (boolean) callWebViewMethod(
                "onKeyMultiple", int.class, int.class, KeyEvent.class, keyCode, repeatCount, event);
    }

    @Override
    public AccessibilityNodeProvider getAccessibilityNodeProvider() {
        AccessibilityNodeProvider provider =
                (AccessibilityNodeProvider) callWebViewMethod("getAccessibilityNodeProvider");
        return provider == null ? super.getAccessibilityNodeProvider() : provider;
    }

    @Deprecated
    @Override
    public boolean shouldDelayChildPressedState() {
        return (boolean) callWebViewMethod("shouldDelayChildPressedState");
    }

    @Override
    public CharSequence getAccessibilityClassName() {
        return WebView.class.getName();
    }

    @Override
    public void onProvideVirtualStructure(ViewStructure structure) {
        callWebViewMethod("onProvideVirtualStructure", ViewStructure.class, structure);
    }

    //    /** @hide */
    //    @Override
    //    public void onInitializeAccessibilityNodeInfoInternal(AccessibilityNodeInfo info) {
    //        super.onInitializeAccessibilityNodeInfoInternal(info);
    //        callWebViewMethod("onInitializeAccessibilityNodeInfo", info);
    //    }
    //
    //    /** @hide */
    //    @Override
    //    public void onInitializeAccessibilityEventInternal(AccessibilityEvent event) {
    //        super.onInitializeAccessibilityEventInternal(event);
    //        callWebViewMethod("onInitializeAccessibilityEvent", event);
    //    }
    //
    //    /** @hide */
    //    @Override
    //    public boolean performAccessibilityActionInternal(int action, Bundle arguments) {
    //        return callWebViewMethod("performAccessibilityAction", action, arguments);
    //    }
    //
    //    /** @hide */
    //    @Override
    //    protected void onDrawVerticalScrollBar(Canvas canvas, Drawable scrollBar,
    //            int l, int t, int r, int b) {
    //        callWebViewMethod("onDrawVerticalScrollBar", canvas, scrollBar, l, t, r, b);
    //    }

    @Override
    protected void onOverScrolled(int scrollX, int scrollY, boolean clampedX, boolean clampedY) {
        callWebViewMethod("onOverScrolled", int.class, int.class, boolean.class, boolean.class,
                scrollX, scrollY, clampedX, clampedY);
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);
        callWebViewMethod("onWindowVisibilityChanged", int.class, visibility);
    }

    //    @Override
    //    protected void onDraw(Canvas canvas) {
    //        callWebViewMethod("onDraw", canvas);
    //    }

    @Override
    public boolean performLongClick() {
        // TODO this logic lives in the glue layer normally.
        return (getParent() != null ? super.performLongClick() : false);
        // return (boolean) callWebViewMethod("performLongClick");
    }

    @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        callWebViewMethod("onConfigurationChanged", Configuration.class, newConfig);
    }

    /**
     * Creates a new InputConnection for an InputMethod to interact with the WebView.
     * This is similar to {@link View#onCreateInputConnection} but note that WebView
     * calls InputConnection methods on a thread other than the UI thread.
     * If these methods are overridden, then the overriding methods should respect
     * thread restrictions when calling View methods or accessing data.
     */
    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return (InputConnection) callWebViewMethod(
                "onCreateInputConnection", EditorInfo.class, outAttrs);
    }

    @Override
    public boolean onDragEvent(DragEvent event) {
        return (boolean) callWebViewMethod("onDragEvent", DragEvent.class, event);
    }

    @Override
    protected void onVisibilityChanged(View changedView, int visibility) {
        super.onVisibilityChanged(changedView, visibility);
        // This method may be called in the constructor chain, before the WebView provider is
        // created.
        //        ensureProviderCreated();
        callWebViewMethod("onVisibilityChanged", View.class, int.class, changedView, visibility);
    }

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus) {
        callWebViewMethod("onWindowFocusChanged", boolean.class, hasWindowFocus);
        super.onWindowFocusChanged(hasWindowFocus);
    }

    @Override
    protected void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
        callWebViewMethod("onFocusChanged", boolean.class, int.class, Rect.class, focused,
                direction, previouslyFocusedRect);
        super.onFocusChanged(focused, direction, previouslyFocusedRect);
    }

    //    /** @hide */
    //    @Override
    //    protected boolean setFrame(int left, int top, int right, int bottom) {
    //        return callWebViewMethod("setFrame", left, top, right, bottom);
    //    }

    //    @Override
    //    protected void onSizeChanged(int w, int h, int ow, int oh) {
    //        super.onSizeChanged(w, h, ow, oh);
    //        callWebViewMethod("onSizeChanged", w, h, ow, oh);
    //    }

    @Override
    protected void onScrollChanged(int l, int t, int oldl, int oldt) {
        super.onScrollChanged(l, t, oldl, oldt);
        callWebViewMethod(
                "onScrollChanged", int.class, int.class, int.class, int.class, l, t, oldl, oldt);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        return (boolean) callWebViewMethod("dispatchKeyEvent", KeyEvent.class, event);
    }

    @Override
    public boolean requestFocus(int direction, Rect previouslyFocusedRect) {
        callWebViewMethod("requestFocus", int.class, Rect.class, direction, previouslyFocusedRect);
        // TODO this super-call is normally done in the glue-layer.
        return super.requestFocus(direction, previouslyFocusedRect);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        callWebViewMethod("onMeasure", int.class, int.class, widthMeasureSpec, heightMeasureSpec);
    }

    @Override
    public boolean requestChildRectangleOnScreen(View child, Rect rect, boolean immediate) {
        return (boolean) callWebViewMethod("requestChildRectangleOnScreen", View.class, Rect.class,
                boolean.class, child, rect, immediate);
    }

    @Override
    public void setBackgroundColor(int color) {
        callWebViewMethod("setBackgroundColor", int.class, color);
    }

    @Override
    public void setLayerType(int layerType, Paint paint) {
        super.setLayerType(layerType, paint);
        callWebViewMethod("setLayerType", int.class, Paint.class, layerType, paint);
    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
        callWebViewMethod("preDispatchDraw", Canvas.class, canvas);
        super.dispatchDraw(canvas);
    }

    @Override
    public void onStartTemporaryDetach() {
        super.onStartTemporaryDetach();
        callWebViewMethod("onStartTemporaryDetach");
    }

    @Override
    public void onFinishTemporaryDetach() {
        super.onFinishTemporaryDetach();
        callWebViewMethod("onFinishTemporaryDetach");
    }

    @Override
    public Handler getHandler() {
        return (Handler) callWebViewMethod("getHandler", Handler.class, super.getHandler());
    }

    @Override
    public View findFocus() {
        return (View) callWebViewMethod("findFocus", View.class, super.findFocus());
    }
}
