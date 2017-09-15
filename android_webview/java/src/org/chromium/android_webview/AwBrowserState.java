// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.android_webview;

import android.annotation.SuppressLint;
import android.content.SharedPreferences;

import org.chromium.base.BuildConfig;
import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;

public class AwBrowserState {
    // We can't reference a Context (which AwBrowserContext references) from a static variable
    // without getting a lint warning - the Context in question is the application context though.
    @SuppressLint("StaticFieldLeak")
    private static AwBrowserContext sBrowserContext;

    AwBrowserState() {}

    // Only on UI thread.
    public static AwBrowserContext createBrowserContextOnUiThread(SharedPreferences webviewPrefs) {
        if (BuildConfig.DCHECK_IS_ON && !ThreadUtils.runningOnUiThread()) {
            throw new RuntimeException(
                    "getBrowserContextOnUiThread called on " + Thread.currentThread());
        }

        if (sBrowserContext == null) {
            sBrowserContext =
                    new AwBrowserContext(webviewPrefs, ContextUtils.getApplicationContext());
        }
        return sBrowserContext;
    }

    public static AwBrowserContext getBrowserContextOnUiThread() {
        if (sBrowserContext == null) throw new RuntimeException("Yolo, sBrowserContext is null.");
        return sBrowserContext;
    }
}
