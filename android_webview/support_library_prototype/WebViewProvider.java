package com.android.webviewsupportlib;

import java.lang.reflect.InvocationHandler;

/**
 * Created by gsennton on 9/20/17.
 */

public interface WebViewProvider {
    void insertVisualStateCallback(long requestId, /* AugmentedWebView.VisualStateCallback */ InvocationHandler callbackInvoHandler);
}
