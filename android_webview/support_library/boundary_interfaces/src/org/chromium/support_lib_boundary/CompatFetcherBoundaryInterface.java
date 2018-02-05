package org.chromium.support_lib_boundary;

import android.webkit.WebSettings;
import android.webkit.WebView;

import java.lang.reflect.InvocationHandler;

public interface CompatFetcherBoundaryInterface {
    Object getAttachedCompat(WebView webview);
    void attachCompat(WebView webview, Object webviewProviderAdapter);

    Object getAttachedCompat(WebSettings webSettings);
    void attachCompat(WebSettings webSettings, Object webSettingsAdapter);
    InvocationHandler createCompat(WebSettings webSettings);
}
