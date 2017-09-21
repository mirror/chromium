package com.android.webviewsupportlib;

import java.lang.reflect.InvocationHandler;

/**
 * Created by gsennton on 9/20/17.
 */

public interface WebViewProviderFactory {
    InvocationHandler /* support_lib.WebViewProvider */ createWebView(android.webkit.WebViewProvider webviewProvider);
}
