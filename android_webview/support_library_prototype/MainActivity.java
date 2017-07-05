package com.android.support_library_prototype;

import static android.view.View.LAYER_TYPE_SOFTWARE;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.webkit.WebView;

public class MainActivity extends Activity {
    // WebView webview;
    private final String TAG = MainActivity.class.getSimpleName();
    SupportLibWebView mWebview;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //        loadNormalWebView();

        loadSupportLibWebView();
    }

    private void loadNormalWebView() {
        WebView webview = new WebView(this);
        webview.setLayerType(LAYER_TYPE_SOFTWARE, null);
        webview.setWebViewClient(new android.webkit.WebViewClient() {
            @Override
            public void onPageFinished(WebView webview, String url) {
                Log.e(TAG, "in onPageFinished. Ehrmagerhd, this works :D");
                super.onPageFinished(webview, url);
            }

            @Override
            public void onReceivedError(WebView view, android.webkit.WebResourceRequest request,
                    android.webkit.WebResourceError error) {
                Log.e(TAG,
                        "in onReceivedError, url: " + request.getUrl().toString() + ", method: "
                                + request.getMethod()); // TODO wasn't allowed to use e.g.
                                                        // isRedirect because of API level.
                super.onReceivedError(view, request, error);
            }
        });
        // The following web page causes onReceivedError.
        // webview.loadUrl("http://www.asajlkdjakldsajdlkdsaj.com/");
        webview.loadUrl("http://www.google.com");
        setContentView(webview);
    }

    private void loadSupportLibWebView() {
        // TODO, ugly hack to cause startChromiumLocked to be called.
        WebView.findAddress("yolo");

        mWebview = new SupportLibWebView(this);
        mWebview.setLayerType(LAYER_TYPE_SOFTWARE, null);
        mWebview.setWebViewClient(new WebViewClient() {
            @Override
            void onPageFinished(String url) {
                Log.e(TAG, "in onPageFinished. Ehrmagerhd, this works :D");
                super.onPageFinished(url);
            }

            @Override
            void onReceivedError(WebResourceRequest request, WebResourceError error) {
                Log.e(TAG,
                        "in onReceivedError, url: " + request.getUrl().toString() + " isRedirect "
                                + request.isRedirect() + ", method: " + request.getMethod()
                                + ", error: " + error.getErrorCode()
                                + ", error description: " + error.getDescription());
                super.onReceivedError(request, error);
            }
        });
        // The following web page causes onReceivedError.
        // mWebview.loadUrl("http://www.asajlkdjakldsajdlkdsaj.com/");
        mWebview.loadUrl("http://www.google.com");
        setContentView(mWebview);
    }
}
