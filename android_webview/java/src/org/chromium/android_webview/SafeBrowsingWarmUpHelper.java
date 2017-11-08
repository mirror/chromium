// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.content.Context;

import org.chromium.base.Callback;

/**
 * A helper class for warming up and cooling down the SafeBrowsing connection for WebView.
 */
public class SafeBrowsingWarmUpHelper {
    public void warmUpSafeBrowsing(Context context, final Callback<Boolean> callback) {
        if (callback != null) callback.onResult(null);
    }
}
