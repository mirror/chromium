package com.android.webviewsupportlibonlynewapisprototype;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.webkit.WebView;

import com.android.webviewsupportlib.AugmentedWebView;

public class MainActivity extends Activity {
    private final static String TAG = MainActivity.class.getSimpleName();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        WebView webview = new WebView(this);
        AugmentedWebView betterWebview = new AugmentedWebView(webview);

        setContentView(webview);

        webview.loadUrl("http://www.google.com");

        betterWebview.postVisualStateCallback(14, new AugmentedWebView.VisualStateCallback() {
            @Override
            public void onComplete(long requestId) {
                Log.e(TAG, "yeah boi, in VisualStateCallback.onComplete! RequestId: " + requestId);
            }
        });

        webview.loadUrl("about:blank");
    }
}
