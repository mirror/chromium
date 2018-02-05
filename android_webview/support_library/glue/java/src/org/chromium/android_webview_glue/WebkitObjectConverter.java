// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.android_webview_glue;

import android.webkit.WebViewProvider;

import com.android.webview.chromium.WebViewChromium;

// TODO there's not really any point to this class - it doesn't put a block between the two glue
// layers..
// TODO we could put this class in the old glue layer, and have it return only Aw-objects that we
// can create support-library clsases from..

// TODO if we want to this to access package-private APIs in old glue it must live in old glue, but
// if we want it to create support-library glue objects it needs access to that as well..
class WebkitObjectConverter {
    public static GlueWebViewChromiumProvider convertWebViewProvider(WebViewProvider provider) {
        WebViewChromium webviewChromium = (WebViewChromium) provider;
        return new GlueWebViewChromiumProvider(
                webviewChromium.getAwContents(), webviewChromium.getAwSettings());
    }
}
