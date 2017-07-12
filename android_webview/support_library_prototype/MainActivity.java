package com.android.support_library_prototype;

import static android.view.View.LAYER_TYPE_SOFTWARE;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Toast;

public class MainActivity extends Activity {
    // WebView webview;
    private final String TAG = MainActivity.class.getSimpleName();

    private LinearLayout mMainLayout;
    private Button mWebviewButton;
    SupportLibWebView mWebview;
    private Button mSuppWebviewButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        mMainLayout = (LinearLayout) findViewById(R.id.activity_main);
        mWebviewButton = (Button) findViewById(R.id.webview_button);
        mWebviewButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                loadNormalWebView();
            }
        });

        mSuppWebviewButton = (Button) findViewById(R.id.supp_webview_button);
        mSuppWebviewButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                loadSupportLibWebView();
            }
        });
    }

    private void loadNormalWebView() {
        WebView webview = new WebView(this);
        //        webview.setLayerType(LAYER_TYPE_SOFTWARE, null);
        webview.setWebViewClient(new android.webkit.WebViewClient() {
            @Override
            public void onPageFinished(WebView webview, String url) {
                Log.e(TAG, "in onPageFinished. Ehrmagerhd, this works :D");
                Toast t = Toast.makeText(
                        MainActivity.this, "in onPageFinished " + url, Toast.LENGTH_SHORT);
                t.show();
                Log.e(TAG, "w " + webview.getWidth() + " h " + webview.getHeight());
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

        //        setContentView(webview);
        mMainLayout.addView(webview);
    }

    private void loadSupportLibWebView() {
        // TODO, ugly hack to cause startChromiumLocked to be called.
        WebView.findAddress("yolo");

        mWebview = new SupportLibWebView(this);
        // TODO we need to set the height of the SupportLibraryWebView explicitly - otherwise it's 0
        // and we won't draw anything on screen.
        mWebview.setLayoutParams(new ViewGroup.LayoutParams(1600, 900));
        //        mWebview.setLayerType(LAYER_TYPE_SOFTWARE, null);
        mWebview.setWebViewClient(new WebViewClient() {
            @Override
            void onPageFinished(String url) {
                Log.e(TAG, "in onPageFinished. Ehrmagerhd, this works :D");
                Toast t = Toast.makeText(
                        MainActivity.this, "in onPageFinished " + url, Toast.LENGTH_SHORT);
                t.show();
                Log.e(TAG, "w " + mWebview.getWidth() + " h " + mWebview.getHeight());
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
        //        setContentView(mWebview);
        mMainLayout.addView(mWebview);
    }
}
