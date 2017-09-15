package com.android.webviewsupportlib;

/**
 * Created by gsennton on 9/13/17.
 */

public class WebViewClient {
    public void onPageFinished(WebView view, String url) {}
    public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
        return false;
    }
}
