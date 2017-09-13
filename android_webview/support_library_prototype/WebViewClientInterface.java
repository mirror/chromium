package com.android.webviewsupportlib;

/**
 * Created by gsennton on 8/24/17.
 */

// TODO we might want this interface to use InvocationHandlers as method parameters...so
// WebViewClient can't override this interface.
public interface WebViewClientInterface { void onPageFinished(WebView view, String url); }
