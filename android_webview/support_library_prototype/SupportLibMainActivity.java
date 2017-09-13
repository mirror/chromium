package com.android.webviewsupportlibprototype;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

import com.android.webviewsupportlib.WebView;
import com.android.webviewsupportlib.WebViewClient;

public class SupportLibMainActivity extends Activity {
    private static final String TAG = SupportLibMainActivity.class.getSimpleName();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_support_lib_main);

        // Just to initialize WebViewChromiumFactoryProvider :D
        android.webkit.GeolocationPermissions.getInstance();

        WebView webview = new WebView(this);
        webview.setWebViewClient(new WebViewClient() {
            @Override
            public void onPageFinished(WebView view, String url) {
                Log.e(TAG, "yolo, we're in onPageFinished " + view + " " + url);
                super.onPageFinished(view, url);
            }
        });
        webview.loadUrl("http://www.google.com");
    }
}
