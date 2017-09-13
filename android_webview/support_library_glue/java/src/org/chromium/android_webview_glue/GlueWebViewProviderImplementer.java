// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.android_webview_glue;

import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.graphics.Paint;
import android.graphics.Picture;
import android.graphics.Rect;
import android.net.http.SslCertificate;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.print.PrintDocumentAdapter;
import android.util.SparseArray;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityNodeProvider;
import android.view.autofill.AutofillValue;
import android.view.DragEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStructure;

import com.android.webviewsupportlib.DownloadListener;
import com.android.webviewsupportlib.ValueCallback;
import com.android.webviewsupportlib.WebBackForwardList;
import com.android.webviewsupportlib.WebChromeClient;
import com.android.webviewsupportlib.WebIconDatabase;
import com.android.webviewsupportlib.WebMessage;
import com.android.webviewsupportlib.WebMessagePort;
import com.android.webviewsupportlib.WebSettings;
import com.android.webviewsupportlib.WebView;
import com.android.webviewsupportlib.WebViewClient;
import com.android.webviewsupportlib.WebViewClientInterface;
import com.android.webviewsupportlib.WebViewProvider;

import java.io.BufferedWriter;
import java.io.File;
import java.lang.reflect.InvocationHandler;
import java.util.Map;

abstract class GlueWebViewProviderImplementer
        implements WebViewProvider, WebViewProvider.ViewDelegate {
    @Override
    public void init(Map<String, Object> javaScriptInterfaces, boolean privateBrowsing) {}

    @Override
    public void setHorizontalScrollbarOverlay(boolean overlay) {}

    @Override
    public void setVerticalScrollbarOverlay(boolean overlay) {}

    @Override
    public boolean overlayHorizontalScrollbar() {
        return false;
    }

    @Override
    public boolean overlayVerticalScrollbar() {
        return false;
    }

    @Override
    public int getVisibleTitleHeight() {
        return 0;
    }

    @Override
    public SslCertificate getCertificate() {
        return null;
    }

    @Override
    public void setCertificate(SslCertificate certificate) {}

    @Override
    public void savePassword(String host, String username, String password) {}

    @Override
    public void setHttpAuthUsernamePassword(
            String host, String realm, String username, String password) {}

    @Override
    public String[] getHttpAuthUsernamePassword(String host, String realm) {
        return new String[0];
    }

    @Override
    public void destroy() {}

    @Override
    public void setNetworkAvailable(boolean networkUp) {}

    @Override
    public WebBackForwardList saveState(Bundle outState) {
        return null;
    }

    @Override
    public boolean savePicture(Bundle b, File dest) {
        return false;
    }

    @Override
    public boolean restorePicture(Bundle b, File src) {
        return false;
    }

    @Override
    public WebBackForwardList restoreState(Bundle inState) {
        return null;
    }

    @Override
    public void loadUrl(String url, Map<String, String> additionalHttpHeaders) {}

    @Override
    public void loadUrl(String url) {}

    @Override
    public void postUrl(String url, byte[] postData) {}

    @Override
    public void loadData(String data, String mimeType, String encoding) {}

    @Override
    public void loadDataWithBaseURL(
            String baseUrl, String data, String mimeType, String encoding, String historyUrl) {}

    @Override
    public void evaluateJavaScript(String script, ValueCallback<String> resultCallback) {}

    @Override
    public void saveWebArchive(String filename) {}

    @Override
    public void saveWebArchive(String basename, boolean autoname, ValueCallback<String> callback) {}

    @Override
    public void stopLoading() {}

    @Override
    public void reload() {}

    @Override
    public boolean canGoBack() {
        return false;
    }

    @Override
    public void goBack() {}

    @Override
    public boolean canGoForward() {
        return false;
    }

    @Override
    public void goForward() {}

    @Override
    public boolean canGoBackOrForward(int steps) {
        return false;
    }

    @Override
    public void goBackOrForward(int steps) {}

    @Override
    public boolean isPrivateBrowsingEnabled() {
        return false;
    }

    @Override
    public boolean pageUp(boolean top) {
        return false;
    }

    @Override
    public boolean pageDown(boolean bottom) {
        return false;
    }

    @Override
    public void insertVisualStateCallback(long requestId, WebView.VisualStateCallback callback) {}

    @Override
    public void clearView() {}

    @Override
    public Picture capturePicture() {
        return null;
    }

    @Override
    public PrintDocumentAdapter createPrintDocumentAdapter(String documentName) {
        return null;
    }

    @Override
    public float getScale() {
        return 0;
    }

    @Override
    public void setInitialScale(int scaleInPercent) {}

    @Override
    public void invokeZoomPicker() {}

    @Override
    public WebView.HitTestResult getHitTestResult() {
        return null;
    }

    @Override
    public void requestFocusNodeHref(Message hrefMsg) {}

    @Override
    public void requestImageRef(Message msg) {}

    @Override
    public String getUrl() {
        return null;
    }

    @Override
    public String getOriginalUrl() {
        return null;
    }

    @Override
    public String getTitle() {
        return null;
    }

    @Override
    public Bitmap getFavicon() {
        return null;
    }

    @Override
    public String getTouchIconUrl() {
        return null;
    }

    @Override
    public int getProgress() {
        return 0;
    }

    @Override
    public int getContentHeight() {
        return 0;
    }

    @Override
    public int getContentWidth() {
        return 0;
    }

    @Override
    public void pauseTimers() {}

    @Override
    public void resumeTimers() {}

    @Override
    public void onPause() {}

    @Override
    public void onResume() {}

    @Override
    public boolean isPaused() {
        return false;
    }

    @Override
    public void freeMemory() {}

    @Override
    public void clearCache(boolean includeDiskFiles) {}

    @Override
    public void clearFormData() {}

    @Override
    public void clearHistory() {}

    @Override
    public void clearSslPreferences() {}

    @Override
    public WebBackForwardList copyBackForwardList() {
        return null;
    }

    @Override
    public void setFindListener(WebView.FindListener listener) {}

    @Override
    public void findNext(boolean forward) {}

    @Override
    public int findAll(String find) {
        return 0;
    }

    @Override
    public void findAllAsync(String find) {}

    @Override
    public boolean showFindDialog(String text, boolean showIme) {
        return false;
    }

    @Override
    public void clearMatches() {}

    @Override
    public void documentHasImages(Message response) {}

    @Override
    public void setWebViewClient(InvocationHandler client) {}

    @Override
    public WebViewClient getWebViewClient() {
        return null;
    }

    @Override
    public void setDownloadListener(DownloadListener listener) {}

    @Override
    public void setWebChromeClient(WebChromeClient client) {}

    @Override
    public WebChromeClient getWebChromeClient() {
        return null;
    }

    @Override
    public void setPictureListener(WebView.PictureListener listener) {}

    @Override
    public void addJavascriptInterface(Object obj, String interfaceName) {}

    @Override
    public void removeJavascriptInterface(String interfaceName) {}

    @Override
    public WebMessagePort[] createWebMessageChannel() {
        return new WebMessagePort[0];
    }

    @Override
    public void postMessageToMainFrame(WebMessage message, Uri targetOrigin) {}

    @Override
    public WebSettings getSettings() {
        return null;
    }

    @Override
    public void setMapTrackballToArrowKeys(boolean setMap) {}

    @Override
    public void flingScroll(int vx, int vy) {}

    @Override
    public View getZoomControls() {
        return null;
    }

    @Override
    public boolean canZoomIn() {
        return false;
    }

    @Override
    public boolean canZoomOut() {
        return false;
    }

    @Override
    public boolean zoomBy(float zoomFactor) {
        return false;
    }

    @Override
    public boolean zoomIn() {
        return false;
    }

    @Override
    public boolean zoomOut() {
        return false;
    }

    @Override
    public void dumpViewHierarchyWithProperties(BufferedWriter out, int level) {}

    @Override
    public View findHierarchyView(String className, int hashCode) {
        return null;
    }

    @Override
    public void setRendererPriorityPolicy(
            int rendererRequestedPriority, boolean waivedWhenNotVisible) {}

    @Override
    public int getRendererRequestedPriority() {
        return 0;
    }

    @Override
    public boolean getRendererPriorityWaivedWhenNotVisible() {
        return false;
    }

    @Override
    public InvocationHandler getViewDelegate() {
        return null;
    }

    @Override
    public ScrollDelegate getScrollDelegate() {
        return null;
    }

    @Override
    public void notifyFindDialogDismissed() {}

    // ViewDelegate methods

    @Override
    public boolean shouldDelayChildPressedState() {
        return false;
    }

    @Override
    public void onProvideVirtualStructure(ViewStructure structure) {}

    @Override
    public void onProvideAutofillVirtualStructure(ViewStructure structure, int flags) {}

    @Override
    public void autofill(SparseArray<AutofillValue> values) {}

    @Override
    public AccessibilityNodeProvider getAccessibilityNodeProvider() {
        return null;
    }

    @Override
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {}

    @Override
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {}

    @Override
    public boolean performAccessibilityAction(int action, Bundle arguments) {
        return false;
    }

    @Override
    public void setOverScrollMode(int mode) {}

    @Override
    public void setScrollBarStyle(int style) {}

    @Override
    public void onDrawVerticalScrollBar(
            Canvas canvas, Drawable scrollBar, int l, int t, int r, int b) {}

    @Override
    public void onOverScrolled(int scrollX, int scrollY, boolean clampedX, boolean clampedY) {}

    @Override
    public void onWindowVisibilityChanged(int visibility) {}

    @Override
    public void onDraw(Canvas canvas) {}

    @Override
    public void setLayoutParams(ViewGroup.LayoutParams layoutParams) {}

    @Override
    public boolean performLongClick() {
        return false;
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {}

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return null;
    }

    @Override
    public boolean onDragEvent(DragEvent event) {
        return false;
    }

    @Override
    public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event) {
        return false;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        return false;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        return false;
    }

    @Override
    public void onAttachedToWindow() {}

    @Override
    public void onDetachedFromWindow() {}

    @Override
    public void onMovedToDisplay(int displayId, Configuration config) {}

    @Override
    public void onVisibilityChanged(View changedView, int visibility) {}

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus) {}

    @Override
    public void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {}

    @Override
    public boolean setFrame(int left, int top, int right, int bottom) {
        return false;
    }

    @Override
    public void onSizeChanged(int w, int h, int ow, int oh) {}

    @Override
    public void onScrollChanged(int l, int t, int oldl, int oldt) {}

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        return false;
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        return false;
    }

    @Override
    public boolean onHoverEvent(MotionEvent event) {
        return false;
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        return false;
    }

    @Override
    public boolean onTrackballEvent(MotionEvent ev) {
        return false;
    }

    @Override
    public boolean requestFocus(int direction, Rect previouslyFocusedRect) {
        return false;
    }

    @Override
    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {}

    @Override
    public boolean requestChildRectangleOnScreen(View child, Rect rect, boolean immediate) {
        return false;
    }

    @Override
    public void setBackgroundColor(int color) {}

    @Override
    public void setLayerType(int layerType, Paint paint) {}

    @Override
    public void preDispatchDraw(Canvas canvas) {}

    @Override
    public void onStartTemporaryDetach() {}

    @Override
    public void onFinishTemporaryDetach() {}

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {}

    @Override
    public Handler getHandler(Handler originalHandler) {
        return null;
    }

    @Override
    public View findFocus(View originalFocusedView) {
        return null;
    }
}
