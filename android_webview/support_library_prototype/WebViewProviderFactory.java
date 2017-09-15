package com.android.webviewsupportlib;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.view.ViewGroup;

import java.lang.reflect.InvocationHandler;
import java.util.List;

/**
 * Created by gsennton on 8/10/17.
 */

public interface WebViewProviderFactory {
    /**
     * This Interface provides glue for implementing the backend of WebView static methods which
     * cannot be implemented in-situ in the proxy class.
     */
    interface Statics {
        /**
         * Implements the API method:
         * {@link android.webkit.WebView#findAddress(String)}
         */
        String findAddress(String addr);

        /**
         * Implements the API method:
         * {@link android.webkit.WebSettings#getDefaultUserAgent(Context) }
         */
        String getDefaultUserAgent(Context context);

        /**
         * Used for tests only.
         */
        void freeMemoryForTests();

        /**
         * Implements the API method:
         * {@link android.webkit.WebView#setWebContentsDebuggingEnabled(boolean) }
         */
        void setWebContentsDebuggingEnabled(boolean enable);

        /**
         * Implements the API method:
         * {@link android.webkit.WebView#clearClientCertPreferences(Runnable) }
         */
        void clearClientCertPreferences(Runnable onCleared);

        /**
         * Implements the API method:
         * {android.webkit.WebView#setSlowWholeDocumentDrawEnabled(boolean) }
         */
        void enableSlowWholeDocumentDraw();

        /**
         * Implement the API method
         * {android.webkit.WebChromeClient.FileChooserParams#parseResult(int, Intent)}
         */
        Uri[] parseFileChooserResult(int resultCode, Intent intent);

        /**
         * Implement the API method
         * {android.webkit.WebView#initSafeBrowsing(Context , ValueCallback<Boolean>)}
         */
        void initSafeBrowsing(Context context, ValueCallback<Boolean> callback);

        /**
         * Implement the API method
         * {android.webkit.WebView#shutdownSafeBrowsing()}
         */
        void shutdownSafeBrowsing();

        /**
         * Implement the API method
         * {android.webkit.WebView#setSafeBrowsingWhitelist(List<String>,
         * ValueCallback<Boolean>)}
         */
        void setSafeBrowsingWhitelist(List<String> urls, ValueCallback<Boolean> callback);
    }

    Statics getStatics();

    /**
     * Construct a new WebViewProvider.
     * @param webView the WebView instance bound to this implementation instance. Note it will not
     * necessarily be fully constructed at the point of this call: defer real initialization to
     * WebViewProvider.init().
     * @param privateAccess provides access into WebView internal methods.
     */
    // TODO using invocation handlers here so glue layer can use interfaces from its own class
    // loader
    // WebViewProvider createWebView(WebView webView, SupportLibWebView.PrivateAccess
    // privateAccess);
    InvocationHandler /* WebViewProvider */ createWebView(InvocationHandler webView,
            InvocationHandler privateAccess, ViewGroup containerView, Context context);

    /**
     * Gets the singleton GeolocationPermissions instance for this WebView implementation. The
     * implementation must return the same instance on subsequent calls.
     * @return the single GeolocationPermissions instance.
     */
    GeolocationPermissions getGeolocationPermissions();

    /**
     * Gets the singleton CookieManager instance for this WebView implementation. The
     * implementation must return the same instance on subsequent calls.
     *
     * @return the singleton CookieManager instance
     */
    // TODO this should be com.android.webviewsupportlib.CookieManager
    CookieManager getCookieManager();

    /**
     * Gets the TokenBindingService instance for this WebView implementation. The
     * implementation must return the same instance on subsequent calls.
     *
     * @return the TokenBindingService instance
     */
    TokenBindingService getTokenBindingService();

    /**
     * Gets the ServiceWorkerController instance for this WebView implementation. The
     * implementation must return the same instance on subsequent calls.
     *
     * @return the ServiceWorkerController instance
     */
    ServiceWorkerController getServiceWorkerController();

    /**
     * Gets the singleton WebIconDatabase instance for this WebView implementation. The
     * implementation must return the same instance on subsequent calls.
     *
     * @return the singleton WebIconDatabase instance
     */
    WebIconDatabase getWebIconDatabase();

    /**
     * Gets the singleton WebStorage instance for this WebView implementation. The
     * implementation must return the same instance on subsequent calls.
     *
     * @return the singleton WebStorage instance
     */
    WebStorage getWebStorage();

    /**
     * Gets the singleton WebViewDatabase instance for this WebView implementation. The
     * implementation must return the same instance on subsequent calls.
     *
     * @return the singleton WebViewDatabase instance
     */
    WebViewDatabase getWebViewDatabase(Context context);
}
