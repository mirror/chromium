// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.android_webview_glue;

import android.net.Uri;

import com.android.webview.chromium.WebViewChromiumFactoryProvider;

import org.chromium.android_webview.AwContentsStatics;
import org.chromium.support_lib_boundary.WVFPStaticsBoundaryInterface;

class GlueWCFPStaticsAdapter implements WVFPStaticsBoundaryInterface {
    WebViewChromiumFactoryProvider mFactory;

    GlueWCFPStaticsAdapter(WebViewChromiumFactoryProvider factory) {
        mFactory = factory;
    }

    public Uri getSafeBrowsingPrivacyPolicyUrl() {
        return AwContentsStatics.getSafeBrowsingPrivacyPolicyUrl();
    }
}
